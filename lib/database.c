#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "io.h"
#include "database.h"
#include "commons.h"

extern int errno;

/* BEGIN - Utility Datatypes */
typedef struct Counter {
    id_t curr_max;
    pthread_mutex_t mutex;
} Counter;
/* BEGIN - Utility Datatypes */

/* BEGIN - Global Variables */
Counter _c_aid;
Counter _c_uid;
Counter _c_tid;
/* END - Global Variables */

/* BEGIN - Helper functions */
int append_lock(int fd, short lock_type, int size){
    if (lock_type == F_UNLCK) {
        size *= -1;
    }

    struct flock flk = {
        .l_type = lock_type,
        .l_whence = SEEK_END,
        .l_start = 0,
        .l_len = size
    };

    return fcntl(fd, F_SETLKW, &flk);
}

int rec_lock(int fd, short lock_type, int start, int len){
    struct flock flk = {
        .l_type = lock_type,
        .l_whence = SEEK_SET,
        .l_start = start,
        .l_len = len
    };

    return fcntl(fd, F_SETLKW, &flk);
}

int init_aid() {
    _c_aid.curr_max = 0;
    if (pthread_mutex_init(&_c_aid.mutex, NULL) == -1) {
        syserr(AT);
        return EFAIL;
    }

    int fd = open(DB_ACCOUNT_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    AccountModel iter;
    id_t max = 0;
    int len;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &iter, sizeof(iter)))) {
        if (len == -1) {
            syserr(AT);
        }

        if (iter.id > max) {
            max = iter.id;
        }
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (close(fd) == -1) {
        syserr(AT);
    }

    _c_aid.curr_max = max;

    return 0;
}

// Never fails
int new_aid() {
    pthread_mutex_lock(&_c_aid.mutex);

    _c_aid.curr_max = _c_aid.curr_max + 1;
    int aid = _c_aid.curr_max;

    pthread_mutex_unlock(&_c_aid.mutex);

    return aid;
}

int init_uid() {
    _c_uid.curr_max = 0;
    if (pthread_mutex_init(&_c_uid.mutex, NULL) == -1) {
        syserr(AT);
        return EFAIL;
    }

    int fd = open(DB_USER_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    UserModel iter;
    id_t max = 0;
    int len;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &iter, sizeof(iter)))) {
        if (len == -1) {
            syserr(AT);
        }

        if (iter.id > max) {
            max = iter.id;
        }
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (close(fd) == -1) {
        syserr(AT);
    }

    _c_uid.curr_max = max;

    return 0;
}

// Never fails
int new_uid() {
    pthread_mutex_lock(&_c_uid.mutex);

    _c_uid.curr_max = _c_uid.curr_max + 1;
    int uid = _c_uid.curr_max;

    pthread_mutex_unlock(&_c_uid.mutex);

    return uid;
}

int init_tid() {
    _c_tid.curr_max = 0;
    if (pthread_mutex_init(&_c_tid.mutex, NULL) == -1) {
        syserr(AT);
        return EFAIL;
    }

    int fd = open(DB_TRANS_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    TransactionModel iter;
    id_t max = 0;
    int len;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = read(fd, &iter, sizeof(iter)))) {
        if (len == -1) {
            syserr(AT);
        }

        if (iter.id > max) {
            max = iter.id;
        }
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (close(fd) == -1) {
        syserr(AT);
    }

    _c_tid.curr_max = max;

    return 0;
}

// Never fails
int new_tid() {
    pthread_mutex_lock(&_c_tid.mutex);

    _c_tid.curr_max = _c_tid.curr_max + 1;
    int tid = _c_tid.curr_max;

    pthread_mutex_unlock(&_c_tid.mutex);

    return tid;
}
/* END - Helper functions */

/* BEGIN - Interface implementation */
int db_init() {
    if (mkdir(DB_BASE_PATH, 0777) == -1) {
        if (errno != EEXIST) {
            syserr(AT);
            return EFAIL;
        }
    }

    int fd;

    // Setup user file and counter
    if ((fd = open(DB_USER_PATH, O_RDONLY | O_CREAT | O_EXCL, 0650)) == -1) {
        if (errno != EEXIST) {
            syserr(AT);
            return EFAIL;
        }
    } else if (close(fd) == -1) {
        syserr(AT);
    }
    if (init_uid() == EFAIL) {
        return EFAIL;
    }

    // Setup account file and counter
    if ((fd = open(DB_ACCOUNT_PATH, O_RDONLY | O_CREAT | O_EXCL, 0650)) == -1) {
        if (errno != EEXIST) {
            syserr(AT);
            return EFAIL;
        }
    } else if (close(fd) == -1) {
        syserr(AT);
    }
    if (init_aid() == EFAIL) {
        return EFAIL;
    }

    // Setup transaction file and counter
    if ((fd = open(DB_TRANS_PATH, O_RDONLY | O_CREAT | O_EXCL, 0650)) == -1) {
        if (errno != EEXIST) {
            syserr(AT);
            return EFAIL;
        }
    } else if (close(fd) == -1) {
        syserr(AT);
    }
    if (init_tid() == EFAIL) {
        return EFAIL;
    }

    return 0;
}

int create_user(UserModel *user) {
    UserModel temp;
    if (get_user_from_uname(user->uname, &temp) != ENOTFOUND) {
        return ECONFLICT;
    }

    int id = new_uid();
    user->id = id;

    int fd = open(DB_USER_PATH, O_WRONLY | O_APPEND);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(UserModel);
    int status = 0;

    append_lock(fd, F_WRLCK, size);
    if (write(fd, user, size) == -1) {
        syserr(AT);
        status = EFAIL;
    }
    append_lock(fd, F_UNLCK, size);

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;
}

// Will ignore changes to uname and aid
int update_user(UserModel *user) {
    int fd = open(DB_USER_PATH, O_RDWR);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(UserModel);
    int status = 0, len;
    UserModel iter;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &iter, size))) {
        if (len == -1) {
            syserr(AT);
            continue;
        }

        if (user->id == iter.id) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (len != 0) {
        off_t pos;
        if ((pos = lseek(fd, -1 * size, SEEK_CUR)) == -1) {
            syserr(AT);
            status = EFAIL;
        } else {
            // Remove changes to fields that can't be changed
            strcpy(user->uname, iter.uname);
            user->aid = iter.aid;

            rec_lock(fd, F_WRLCK, pos, size);
            if (write(fd, user, size) == -1) {
                syserr(AT);
                status = EFAIL;
            }
            rec_lock(fd, F_UNLCK, pos, size);
        }
    } else {
        status = ENOTFOUND;
    }

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;
}

int delete_user(id_t uid) {
    int fd = open(DB_USER_PATH, O_RDWR);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(UserModel);
    int status = 0, len;
    UserModel iter;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &iter, size))) {
        if (len == -1) {
            syserr(AT);
            continue;
        }

        if (uid == iter.id) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (len != 0) {
        off_t lpos;
        if ((lpos = lseek(fd, 0, SEEK_CUR)) == -1) {
            syserr(AT);
        }

        rec_lock(fd, F_WRLCK, lpos, 0);

        while((len = safe_read(fd, &iter, size))) {
            if (len == -1) {
                syserr(AT);
            }

            if(lseek(fd, -2 * size, SEEK_CUR) == -1) {
                syserr(AT);
            }

            if(write(fd, &iter, size) == -1) {
                syserr(AT);
            }

            if(lseek(fd, size, SEEK_CUR) == -1) {
                syserr(AT);
            }
        }

        off_t pos;
        // Last record is repeated twice, truncate file
        if ((pos = lseek(fd, -1 * size, SEEK_END)) == -1) {
            syserr(AT);
        }

        if (ftruncate(fd, pos) == -1) {
            syserr(AT);
        }

        rec_lock(fd, F_UNLCK, lpos, 0);

    } else {
        status = ENOTFOUND;
    }

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;
}

int get_num_users() {
    int fd = open(DB_USER_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    off_t pos;
    if ((pos = lseek(fd, 0, SEEK_END)) == -1) {
        syserr(AT);
        return EFAIL;
    }

    if(close(fd) == -1) {
        syserr(AT);
    }

    return pos / (sizeof(UserModel));
}

int get_users(UserModel *users, int limit) {
    int fd = open(DB_USER_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    int len, i = 0;
    size_t size = sizeof(UserModel);

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, (users + i), size))) {
        if (len == -1) {
            syserr(AT);
        }

        i++;

        if(i == limit) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if(close(fd) == -1) {
        syserr(AT);
    }

    return i;
}

int get_user_from_id(id_t uid, UserModel *user) {
    int fd = open(DB_USER_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(UserModel);
    int status = 0, len;
    UserModel iter;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &iter, size))) {
        if (len == -1) {
            syserr(AT);
            continue;
        }

        if (iter.id == uid) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (len != 0) {
        *user = iter;
    } else {
        status = ENOTFOUND;
    }

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;
}

int get_user_from_uname(char *uname, UserModel *user) {
    int fd = open(DB_USER_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(UserModel);
    int status = 0, len;
    UserModel iter;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &iter, size))) {
        if (len == -1) {
            syserr(AT);
            continue;
        }

        if (strcmp(uname, iter.uname) == 0) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (len != 0) {
        *user = iter;
    } else {
        status = ENOTFOUND;
    }

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;

}

int create_account(AccountModel *account) {
    // Get new account id
    int id = new_aid();
    account->id = id;

    int fd = open(DB_ACCOUNT_PATH, O_WRONLY | O_APPEND);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(AccountModel);
    int status = 0;

    append_lock(fd, F_WRLCK, size);
    if (write(fd, account, size) == -1) {
        syserr(AT);
        status = EFAIL;
    }
    append_lock(fd, F_UNLCK, size);

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;
}

int update_account(AccountModel *account) {
    int fd = open(DB_ACCOUNT_PATH, O_RDWR);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(AccountModel);
    int status = 0, len;
    AccountModel iter;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &iter, size))) {
        if (len == -1) {
            syserr(AT);
            continue;
        }

        if (iter.id == account->id) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (len != 0) {
        off_t pos;
        if ((pos = lseek(fd, -1 * size, SEEK_CUR)) == -1) {
            syserr(AT);
            status = EFAIL;
        } else {
            rec_lock(fd, F_WRLCK, pos, size);
            if (write(fd, account, size) == -1) {
                syserr(AT);
                status = EFAIL;
            }
            rec_lock(fd, F_UNLCK, pos, size);
        }
    } else {
        status = ENOTFOUND;
    }

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;
}

int delete_account(id_t aid) {
    int fd = open(DB_ACCOUNT_PATH, O_RDWR);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(AccountModel);
    int status = 0, len;
    AccountModel iter;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &iter, size))) {
        if (len == -1) {
            syserr(AT);
            continue;
        }

        if (iter.id == aid) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (len != 0) {
        off_t lpos;
        if ((lpos = lseek(fd, 0, SEEK_CUR)) == -1) {
            syserr(AT);
        }

        rec_lock(fd, F_WRLCK, lpos, 0);

        while((len = safe_read(fd, &iter, size))) {
            if (len == -1) {
                syserr(AT);
            }

            if(lseek(fd, -2 * size, SEEK_CUR) == -1) {
                syserr(AT);
            }

            if(write(fd, &iter, size) == -1) {
                syserr(AT);
            }

            if(lseek(fd, size, SEEK_CUR) == -1) {
                syserr(AT);
            }
        }

        off_t pos;
        // Last record is repeated twice, truncate file
        if ((pos = lseek(fd, -1 * size, SEEK_END)) == -1) {
            syserr(AT);
        }

        if (ftruncate(fd, pos) == -1) {
            syserr(AT);
        }

        rec_lock(fd, F_UNLCK, lpos, 0);
    } else {
        status = ENOTFOUND;
    }

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;
}

int get_num_accounts() {
    int fd = open(DB_ACCOUNT_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    off_t pos;
    if ((pos = lseek(fd, 0, SEEK_END)) == -1) {
        syserr(AT);
        return EFAIL;
    }

    if(close(fd) == -1) {
        syserr(AT);
    }

    return pos / (sizeof(AccountModel));
}

int get_accounts(AccountModel *accounts, int limit) {
    int fd = open(DB_ACCOUNT_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    int len, i = 0;
    size_t size = sizeof(AccountModel);

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, (accounts + i), size))) {
        if (len == -1) {
            syserr(AT);
        }

        i++;

        if(i == limit) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if(close(fd) == -1) {
        syserr(AT);
    }

    return i;
}

int get_account_from_id(id_t aid, AccountModel *account) {
    int fd = open(DB_ACCOUNT_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(AccountModel);
    int status = 0, len;
    AccountModel iter;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &iter, size))) {
        if (len == -1) {
            syserr(AT);
            continue;
        }

        if (iter.id == aid) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if (len != 0) {
        *account = iter;
    } else {
        status = ENOTFOUND;
    }

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;
}

int append_transaction(TransactionModel *trans) {
    // Get new account id
    int id = new_tid();
    trans->id = id;

    int fd = open(DB_TRANS_PATH, O_WRONLY | O_APPEND);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    size_t size = sizeof(TransactionModel);
    int status = 0;

    append_lock(fd, F_WRLCK, size);
    if (write(fd, trans, size) == -1) {
        syserr(AT);
        status = EFAIL;
    }
    append_lock(fd, F_UNLCK, size);

    if (close(fd) == -1) {
        syserr(AT);
    }

    return status;
}

int get_num_transactions(id_t aid) {
    int fd = open(DB_TRANS_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    int len, i = 0;
    size_t size = sizeof(TransactionModel);
    TransactionModel trans;

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, &trans, size))) {
        if (len == -1) {
            syserr(AT);
        }

        if (trans.aid == aid) i++;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if(close(fd) == -1) {
        syserr(AT);
    }

    return i;
}

int get_transactions(id_t aid, TransactionModel *trans, int limit) {
    int fd = open(DB_TRANS_PATH, O_RDONLY);
    if (fd == -1) {
        syserr(AT);
        return EFAIL;
    }

    int len, i = 0;
    size_t size = sizeof(TransactionModel);

    rec_lock(fd, F_RDLCK, 0, 0);
    while((len = safe_read(fd, (trans + i), size))) {
        if (len == -1) {
            syserr(AT);
        }

        if (trans[i].aid == aid) i++;

        if(i == limit) break;
    }
    rec_lock(fd, F_UNLCK, 0, 0);

    if(close(fd) == -1) {
        syserr(AT);
    }

    return i;
}
/* END - Interface implementation */
