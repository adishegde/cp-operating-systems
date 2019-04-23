#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "lib/communicate.h"
#include "lib/database.h"
#include "lib/io.h"
#include "lib/commons.h"

#define MAX_SESSIONS 100

typedef struct SessionPool {
    id_t list[MAX_SESSIONS];
    pthread_mutex_t mutex;
} SessionPool;

SessionPool spool;
int sock_fd;

// No check for filled pool
id_t new_session(id_t uid) {
    pthread_mutex_lock(&spool.mutex);

    id_t i;
    for(i = 0; i < MAX_SESSIONS; ++i) {
        if (spool.list[i] == 0) {
            spool.list[i] = uid;
            break;
        }
    }

    pthread_mutex_unlock(&spool.mutex);

    return i + 1;
}

id_t get_uid_from_sid(id_t sid) {
    pthread_mutex_lock(&spool.mutex);

    id_t uid = spool.list[sid - 1];

    pthread_mutex_unlock(&spool.mutex);

    return uid;
}

void close_session(id_t sid) {
    pthread_mutex_lock(&spool.mutex);

    spool.list[sid - 1] = 0;

    pthread_mutex_unlock(&spool.mutex);
}

int create_socket(int portno) {
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        syserr(AT);
        return EFAIL;
    }

    struct linger lin;

    int reuse = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof(reuse)) == -1) {
        syserr(AT);
    }

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(portno);

    if (bind(sd, (struct sockaddr*) &serv, sizeof(serv)) == -1) {
        syserr(AT);

        if (close(sd) == -1) {
            syserr(AT);
        }

        return EFAIL;
    }

    if (listen(sd, 5) == -1) {
        syserr(AT);

        if (close(sd) == -1) {
            syserr(AT);
        }

        return EFAIL;
    }

    return sd;
}

void login_controller(int nsd) {
    printf("login_controller - Incoming Request\n");

    LoginRequest req;
    LoginResponse resp;
    resp.stat = Success;

    if (safe_read(nsd, &req, sizeof(req)) == -1) {
        syserr(AT);
    }

    UserModel u;
    if (get_user_from_uname(req.uname, &u) < 0) {
        resp.stat = Failure;
    } else if (strcmp(u.password, req.password) != 0) {
        resp.stat = Unauthorized;
    } else {
        id_t sid = new_session(u.id);
        resp.sid = sid;
        resp.is_admin = u.is_admin;
    }

    printf("login_controller - Response: Status: %d SID: %lu IS_ADMIN: %d\n", resp.stat, resp.sid, resp.is_admin);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }
}

void transaction_controller(int nsd, id_t sid) {
    printf("transaction_controller - Incoming Request: SID: %lu\n", sid);

    TransactionRequest req;
    TransactionResponse resp;
    resp.stat = Success;

    if (safe_read(nsd, &req, sizeof(req)) == -1) {
        syserr(AT);
    }

    id_t uid = get_uid_from_sid(sid);

    UserModel u;
    AccountModel acc;
    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else if (get_account_from_id(u.aid, &acc) < 0) {
        resp.stat = Failure;
    } else if (acc.balance + req.amt < 0) {
        resp.stat = NoFunds;
    } else {
        resp.old_balance = acc.balance;
        acc.balance += req.amt;

        TransactionModel tran;

        if (update_account(&acc) < 0) {
            resp.stat = Failure;
        } else {
            resp.new_balance = acc.balance;

            tran.aid = u.aid;
            tran.uid = u.id;
            tran.old_balance = resp.old_balance;
            tran.new_balance = resp.new_balance;
            tran.when = time(NULL);

            append_transaction(&tran);

            resp.id = tran.id;
        }
    }

    printf("transaction_controller - Response: SID: %lu Status: %d ID: %lu O_BAL: %f N_BAL: %f\n", sid, resp.stat, resp.id, resp.old_balance, resp.new_balance);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }
}

void balance_enquiry_controller(int nsd, id_t sid) {
    printf("balance_enquiry_controller - Incoming Request: SID: %lu\n", sid);

    BalanceEnquiryResponse resp;
    resp.stat = Success;

    id_t uid = get_uid_from_sid(sid);

    UserModel u;
    AccountModel acc;
    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else if (get_account_from_id(u.aid, &acc) < 0) {
        resp.stat = Failure;
    } else {
        resp.balance = acc.balance;
    }

    printf("balance_enquiry_controller - Response: SID: %lu Status: %d BAL: %f\n", sid, resp.stat, resp.balance);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }
}

void password_change_controller(int nsd, id_t sid) {
    printf("password_change_controller - Incoming Request: SID: %lu\n", sid);

    PasswordChangeRequest req;
    PasswordChangeResponse resp;
    resp.stat = Success;

    if (safe_read(nsd, &req, sizeof(req)) == -1) {
        syserr(AT);
    }

    id_t uid = get_uid_from_sid(sid);

    UserModel u;
    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else {
        strcpy(u.password, req.password);

        if (update_user(&u) < 0) {
            resp.stat = Failure;
        }
    }

    printf("password_change_controller - Response: SID: %lu Status: %d Password: %s\n", sid, resp.stat, req.password);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }
}

void view_details_controller(int nsd, id_t sid) {
    printf("view_details_controller - Incoming Request: SID: %lu\n", sid);

    ViewDetailsResponse resp;
    resp.stat = Success;

    id_t uid = get_uid_from_sid(sid);

    UserModel u;
    AccountModel acc;
    int num_trans;

    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else if (get_account_from_id(u.aid, &acc) < 0) {
        resp.stat = Failure;
    } else if ((num_trans = get_num_transactions(u.aid)) < 0) {
        resp.stat = Failure;
    } else {
        resp.uid = u.id;
        resp.aid = u.aid;
        resp.balance = acc.balance;
        resp.is_admin = u.is_admin;
        resp.num_trans = num_trans;

        strcpy(resp.name, u.name);
        strcpy(resp.uname, u.uname);
        strcpy(resp.password, u.password);
    }

    printf("view_details_controller - Response: SID: %lu Status: %d Num_Trans: %d\n", sid, resp.stat, resp.num_trans);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }

    if (resp.stat == Success) {
        TransactionModel *trans = malloc(num_trans * sizeof(TransactionModel));
        TransactionItem tresp;
        tresp.stat = Success;

        if (get_transactions(u.aid, trans, num_trans) < 0) {
            tresp.stat = Failure;

            if (write(nsd, &tresp, sizeof(tresp)) == -1) {
                syserr(AT);
            }
        } else {
            int i;
            for (i = 0; i < num_trans; ++i) {
                tresp.uid = trans[i].uid;
                tresp.id = trans[i].id;
                tresp.when = trans[i].when;
                tresp.old_balance = trans[i].old_balance;
                tresp.new_balance = trans[i].new_balance;

                if (write(nsd, &tresp, sizeof(tresp)) == -1) {
                    syserr(AT);
                }
            }
        }

        free(trans);
    }
}

void logout_controller(int nsd, id_t sid) {
    printf("logout_controller - Incoming Request: SID: %lu\n", sid);

    LogoutResponse resp;
    resp.stat = Success;

    close_session(sid);

    printf("logout_controller - Response: SID: %lu Stat: %d\n", sid, resp.stat);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }
}

void create_user_controller(int nsd, id_t sid) {
    printf("create_user_controller - Incoming Request: SID: %lu\n", sid);

    CreateUserRequest req;
    CreateUserResponse resp;
    resp.stat = Success;

    if (safe_read(nsd, &req, sizeof(req)) == -1) {
        syserr(AT);
    }

    id_t uid = get_uid_from_sid(sid);

    UserModel u;
    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else if (!u.is_admin) {
        resp.stat = Unauthorized;
    } else {
        UserModel new_user;

        strcpy(new_user.name, req.name);
        strcpy(new_user.uname, req.uname);
        strcpy(new_user.password, req.password);
        new_user.is_admin = req.is_admin;
        new_user.aid = req.aid;

        if (req.aid == 0) {
            // Create new account for new user
            AccountModel acc;
            acc.balance = 0;

            create_account(&acc); // No error checks for code simplicity

            new_user.aid = acc.id;
        }

        int ret;
        if((ret = create_user(&new_user)) == ECONFLICT) {
            resp.stat = Conflict;
        } else if (ret == EFAIL) {
            resp.stat = Failure;
        } else {
            resp.uid = new_user.id;
            resp.aid = new_user.aid;
        }
    }

    printf("create_user_controller - Response: SID: %lu Status: %d UID: %lu AID: %lu\n", sid, resp.stat, resp.uid, resp.aid);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }
}

void get_users_controller(int nsd, id_t sid) {
    printf("get_users_controller - Incoming Request: SID: %lu\n", sid);

    GetUsersResponse resp;
    resp.stat = Success;

    id_t uid = get_uid_from_sid(sid);

    UserModel u;

    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else if (!u.is_admin) {
        resp.stat = Unauthorized;
    } else if ((resp.num_users = get_num_users()) == EFAIL) {
        resp.stat = Failure;
    }

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }

    printf("get_users_controller - Response: SID: %lu Status: %d Num_Users: %d\n", sid, resp.stat, resp.num_users);

    if (resp.stat == Success) {
        UserModel *uarr = malloc(resp.num_users * sizeof(UserModel));
        UserItem uresp;
        uresp.stat = Success;

        if (get_users(uarr, resp.num_users) == EFAIL) {
            uresp.stat = Failure;

            if (write(nsd, &uresp, sizeof(uresp)) == -1) {
                syserr(AT);
            }
        } else {
            int i;
            for(i = 0; i < resp.num_users; ++i) {
                strcpy(uresp.name, uarr[i].name);
                strcpy(uresp.uname, uarr[i].uname);
                strcpy(uresp.password, uarr[i].password);
                uresp.is_admin = uarr[i].is_admin;
                uresp.aid = uarr[i].aid;
                uresp.id = uarr[i].id;

                if (write(nsd, &uresp, sizeof(uresp)) == -1) {
                    syserr(AT);
                }
            }
        }

        free(uarr);
    }
}

void delete_user_controller(int nsd, id_t sid) {
    printf("delete_user_controller - Incoming Request: SID: %lu\n", sid);

    DeleteUserRequest req;
    DeleteUserResponse resp;
    resp.stat = Success;

    if (safe_read(nsd, &req, sizeof(req)) == -1) {
        syserr(AT);
    }

    id_t uid = get_uid_from_sid(sid);

    UserModel u;
    int ret;
    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else if (!u.is_admin) {
        resp.stat = Unauthorized;

    } else if (u.id == req.uid) {
        // User can't delete himself
        resp.stat = NotFound;
    } else if ((ret = delete_user(req.uid)) == ENOTFOUND) {
        resp.stat = NotFound;
    } else if (ret < 0) {
        resp.stat = Failure;
    }

    printf("delete_user_controller - Response: SID: %lu Status: %d\n", sid, resp.stat);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }
}

void modify_user_controller(int nsd, id_t sid) {
    printf("modify_user_controller - Incoming Request: SID: %lu\n", sid);

    ModifyUserRequest req;
    ModifyUserResponse resp;
    resp.stat = Success;

    if (safe_read(nsd, &req, sizeof(req)) == -1) {
        syserr(AT);
    }

    id_t uid = get_uid_from_sid(sid);

    UserModel u;
    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else if (!u.is_admin) {
        resp.stat = Unauthorized;
    } else {
        UserModel mod_user;

        strcpy(mod_user.name, req.name);
        strcpy(mod_user.password, req.password);
        mod_user.is_admin = req.is_admin;
        mod_user.id = req.uid;

        int ret;
        if ((ret = update_user(&mod_user)) == ENOTFOUND) {
            resp.stat = NotFound;
        } else if (ret == EFAIL) {
            resp.stat = Failure;
        } else {
            strcpy(resp.name, mod_user.name);
            strcpy(resp.uname, mod_user.uname);
            strcpy(resp.password, mod_user.password);
            resp.is_admin = mod_user.is_admin;
            resp.aid = mod_user.aid;
            resp.id = mod_user.id;
        }
    }

    printf("modify_account_controller - Response: SID: %lu Status: %d UID: %lu\n", sid, resp.stat, req.uid);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }
}

void get_accounts_controller(int nsd, id_t sid) {
    printf("get_accounts_controller - Incoming Request: SID: %lu\n", sid);

    GetAccountsResponse resp;
    resp.stat = Success;

    id_t uid = get_uid_from_sid(sid);

    UserModel u;

    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else if (!u.is_admin) {
        resp.stat = Unauthorized;
    } else if ((resp.num_accounts = get_num_accounts()) == EFAIL) {
        resp.stat = Failure;
    }

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }

    printf("get_accounts_controller - Response: SID: %lu Status: %d Num_Accounts: %d\n", sid, resp.stat, resp.num_accounts);

    if (resp.stat == Success) {
        AccountModel *arr = malloc(resp.num_accounts * sizeof(AccountModel));
        AccountItem aresp;
        aresp.stat = Success;

        if (get_accounts(arr, resp.num_accounts) == EFAIL) {
            aresp.stat = Failure;

            if (write(nsd, &aresp, sizeof(aresp)) == -1) {
                syserr(AT);
            }
        } else {
            int i;
            for(i = 0; i < resp.num_accounts; ++i) {
                aresp.balance = arr[i].balance;
                aresp.id = arr[i].id;

                if (write(nsd, &aresp, sizeof(aresp)) == -1) {
                    syserr(AT);
                }
            }
        }

        free(arr);
    }

}

void delete_account_controller(int nsd, id_t sid) {
    printf("delete_account_controller - Incoming Request: SID: %lu\n", sid);

    DeleteAccountRequest req;
    DeleteAccountResponse resp;
    resp.stat = Success;

    if (safe_read(nsd, &req, sizeof(req)) == -1) {
        syserr(AT);
    }

    id_t uid = get_uid_from_sid(sid);

    UserModel u;
    int ret;
    if (get_user_from_id(uid, &u) < 0) {
        resp.stat = Failure;
    } else if (!u.is_admin) {
        resp.stat = Unauthorized;
    } else if ((ret = delete_account(req.aid)) == ENOTFOUND) {
        resp.stat = NotFound;
    } else if (ret == ECONFLICT) {
        resp.stat = Conflict;
    } else if (ret < 0) {
        resp.stat = Failure;
    }

    printf("delete_user_controller - Response: SID: %lu Status: %d\n", sid, resp.stat);

    if (write(nsd, &resp, sizeof(resp)) == -1) {
        syserr(AT);
    }
}

void* handle_request(void *sock_desc) {
    int nsd = *(int *)sock_desc;

    struct Header header;

    if (safe_read(nsd, &header, sizeof(header)) == -1) {
        syserr(AT);
    }

    switch(header.action) {
    case Login:
        login_controller(nsd);
        break;

    case Transaction:
        transaction_controller(nsd, header.sid);
        break;

    case BalanceEnquiry:
        balance_enquiry_controller(nsd, header.sid);
        break;

    case PasswordChange:
        password_change_controller(nsd, header.sid);
        break;

    case ViewDetails:
        view_details_controller(nsd, header.sid);
        break;

    case Logout:
        logout_controller(nsd, header.sid);
        break;

    case CreateUser:
        create_user_controller(nsd, header.sid);
        break;

    case ModifyUser:
        modify_user_controller(nsd, header.sid);
        break;

    case GetUsers:
        get_users_controller(nsd, header.sid);
        break;

    case DeleteUser:
        delete_user_controller(nsd, header.sid);
        break;

    case GetAccounts:
        get_accounts_controller(nsd, header.sid);
        break;

    case DeleteAccount:
        delete_account_controller(nsd, header.sid);
        break;
    }

    if (close(nsd) == -1) {
        syserr(AT);
    }

    return NULL;
}

int server_init() {
    printf("Initializing db\n");
    if (db_init() == EFAIL) {
        apperr(AT, "Failed to create database. Exiting.\n");
        return EFAIL;
    }

    printf("Starting server\n");

    sock_fd = create_socket(PORTNO);
    if (sock_fd < 0) {
        apperr(AT, "Failed to create socket. Exiting.\n");
        return EFAIL;
    }

    printf("Server listening at port %d and has pid %d\n", PORTNO, getpid());

    if (pthread_mutex_init(&spool.mutex, NULL) == -1) {
        syserr(AT);
        return EFAIL;
    }

    return sock_fd;
}

void kill_server() {
    printf("\nShutting down server\n");

    if (close(sock_fd) == -1) {
        syserr(AT);
    }

    exit(0);
}

int main() {
    if (server_init() == EFAIL) {
        return -1;
    }

    signal(SIGINT, kill_server);

    while (1) {
        int nsd = accept(sock_fd, NULL, NULL);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, &handle_request, &nsd);
        pthread_detach(thread_id);
    }

    if (close(sock_fd) == -1) {
        syserr(AT);
    }

    return 0;
}
