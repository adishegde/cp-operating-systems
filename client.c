#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>

#include "lib/communicate.h"
#include "lib/io.h"
#include "lib/fort.h"

void wait_for_enter() {
    getchar();
    while((getchar()) != '\n');
}

int connect_to_server(int portno) {
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
	syserr(AT);
	return EFAIL;
    }

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(portno);

    if (connect(sd, (struct sockaddr*) &serv, sizeof(serv)) == -1) {
	    syserr(AT);
    }

    return sd;
}

int login(int sd, LoginResponse *resp) {
	Header head = {.action = Login};
	LoginRequest req;

	printf("--- Login ---\n");
	printf("Username: ");
	fgets(req.uname, INFO_STRING_LEN, stdin);
	req.uname[strlen(req.uname) - 1] = '\0';
	printf("Password: ");
	fgets(req.password, INFO_STRING_LEN, stdin);
	req.password[strlen(req.password) - 1] = '\0';
	printf("\n");

	int stat = EFAIL;

	if (write(sd, &head, sizeof(head)) == -1) {
		syserr(AT);
		printf("Communication Error.\n");
		stat = EFAIL;
	} else if (write(sd, &req, sizeof(req)) == -1) {
		syserr(AT);
		printf("Communication Error.\n");
	} else if (safe_read(sd, resp, sizeof(LoginResponse)) == -1) {
		syserr(AT);
		printf("Communication Error.\n");
	} else if (resp->stat != Success){
		printf("Login Unsuccessful.\n");
		stat = EFAIL;
	} else {
		printf("Login Successful.\n");
		printf("Session ID: %lu\n", resp->sid);

		stat = 0;
	}

	printf("--- Login ---\n");

	return stat;
}

void withdraw(int sd, id_t sid) {
	balance_t amt;

	printf("--- Withdraw ---\n");
	printf("Amount: ");
	scanf("%lf", &amt);
	printf("\n");
	amt *= -1; // Negative amt implies withdrawal

	Header head = {.action = Transaction, .sid = sid};
	TransactionRequest treq = {.amt = amt};
	TransactionResponse tresp;

	if (write(sd, &head, sizeof(head)) == -1) {
		syserr(AT);
		printf("Communication Error.\n");
		printf("--- Withdraw ---\n");
		return;
	} else if (write(sd, &treq, sizeof(treq)) == -1) {
		syserr(AT);
		printf("Communication Error.\n");
	}

	if (safe_read(sd, &tresp, sizeof(tresp)) == -1) {
		syserr(AT);
		printf("Communication Error.\n");
	} else if (tresp.stat == Failure) {
		printf("Transaction Failed.\n");
	} else if (tresp.stat == NoFunds) {
		printf("Funds insufficient for withdrawing required amount.\n");
	} else if (tresp.stat != Success) {
		printf("Error occurred.\n");
	} else {
		printf("Transaction Successful.\n");

		ft_table_t *table = ft_create_table();
		ft_set_border_style(table, FT_DOUBLE2_STYLE);
		ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);

		ft_write_ln(table, "Name", "Value");
		ft_printf_ln(table, "%s|%lu", "ID", tresp.id);
		ft_printf_ln(table, "%s|%lf", "Old Balance", tresp.old_balance);
		ft_printf_ln(table, "%s|%lf", "Current Balance", tresp.new_balance);

		printf("%s\n", ft_to_string(table));
		ft_destroy_table(table);
	}

	printf("--- Withdraw ---\n");
}

void deposit(int sd, id_t sid) {
	balance_t amt;

	printf("--- Deposit ---\n");
	printf("Amount: ");
	scanf("%lf", &amt);
	printf("\n");

	Header head = {.action = Transaction, .sid = sid};
	TransactionRequest treq = {.amt = amt};
	TransactionResponse tresp;

	if (write(sd, &head, sizeof(head)) == -1) {
		syserr(AT);
		printf("Communication Error.\n");
		printf("--- Deposit ---\n");
		return;
	} else if (write(sd, &treq, sizeof(treq)) == -1) {
		syserr(AT);
		printf("Communication Error.\n");
	}

	if (safe_read(sd, &tresp, sizeof(tresp)) == -1) {
		syserr(AT);
		printf("Communication Error.\n");
	} else if (tresp.stat == Failure) {
		printf("Transaction Failed.\n");
	} else if (tresp.stat == NoFunds) {
		printf("Funds insufficient for withdrawing required amount.\n");
	} else if (tresp.stat != Success) {
		printf("Error occurred.\n");
	} else {
		printf("Transaction Successful.\n");

		ft_table_t *table = ft_create_table();
		ft_set_border_style(table, FT_DOUBLE2_STYLE);
		ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);

		ft_write_ln(table, "Name", "Value");
		ft_printf_ln(table, "%s|%lu", "ID", tresp.id);
		ft_printf_ln(table, "%s|%lf", "Old Balance", tresp.old_balance);
		ft_printf_ln(table, "%s|%lf", "Current Balance", tresp.new_balance);

		printf("%s\n", ft_to_string(table));
		ft_destroy_table(table);
	}
	printf("--- Deposit ---\n");
}

void balance_enquiry(int sd, id_t sid) {
    printf("--- Balance Enquiry ---\n");

    Header head = {.action = BalanceEnquiry, .sid = sid};
    BalanceEnquiryResponse resp;

    if (write(sd, &head, sizeof(head)) == -1) {
	syserr(AT);
	printf("Communication Error.\n");
    } else if (safe_read(sd, &resp, sizeof(resp)) == -1) {
	syserr(AT);
	printf("Communication Error.\n");
    } else if (resp.stat != Success){
	printf("Request Failed.\n");
    } else {
	ft_table_t *table = ft_create_table();
	ft_set_border_style(table, FT_DOUBLE2_STYLE);

	ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
	ft_write_ln(table, "Balance Details", "");
	ft_set_cell_span(table, 0, 0, 2);
	ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);

	ft_printf_ln(table, "%s|%lf", "Balance", resp.balance);
	printf("%s\n", ft_to_string(table));

	ft_destroy_table(table);
    }

    printf("--- Balance Enquiry ---\n");
}

void view_details(int sd, id_t sid) {
    printf("--- View Details ---\n");

    Header head = {.action = ViewDetails, .sid = sid};
    ViewDetailsResponse resp;

    if (write(sd, &head, sizeof(head)) == -1) {
	syserr(AT);
	printf("Communication Error.\n");
    } else if (safe_read(sd, &resp, sizeof(resp)) == -1) {
	syserr(AT);
	printf("Communication Error.\n");
    } else if (resp.stat != Success) {
	printf("Request Failed.\n");
    } else {
	ft_table_t *table = ft_create_table();

	ft_set_border_style(table, FT_DOUBLE2_STYLE);
	ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
	ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
	ft_set_cell_span(table, 0, 0, 2);
	ft_write_ln(table, "User and Account Details");

	ft_set_cell_prop(table, 1, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
	ft_write_ln(table, "Name", "Value");

	ft_printf_ln(table, "%s|%lu", "User ID", resp.uid);
	ft_printf_ln(table, "%s|%lu", "Account ID", resp.aid);
	ft_printf_ln(table, "%s|%s", "Name", resp.name);
	ft_printf_ln(table, "%s|%s", "Username", resp.uname);
	ft_printf_ln(table, "%s|%s", "Password", resp.password);
	ft_printf_ln(table, "%s|%lf", "Balance", resp.balance);

	printf("%s\n", ft_to_string(table));
	ft_destroy_table(table);

	// Print transactions
	table = ft_create_table();

	ft_set_border_style(table, FT_DOUBLE2_STYLE);
	ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
	ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
	ft_set_cell_span(table, 0, 0, 5);
	ft_write_ln(table, "Transactions");

	ft_set_cell_prop(table, 1, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
	ft_write_ln(table, "ID", "User ID", "Old Balance", "New Balance", "When");

	TransactionItem iter;
	int i;
	char when[50];
	struct tm *timeinfo;
	for(i = 0; i < resp.num_trans; ++i) {
	    if (safe_read(sd, &iter, sizeof(iter)) == -1) {
		syserr(AT);
		continue;
	    }

	    timeinfo = localtime(&iter.when);
	    strftime(when, sizeof(when), "%b %d %H:%M", timeinfo);
	    ft_printf_ln(
		table,
		"%lu|%lu|%lf|%lf|%s",
		iter.id,
		iter.uid,
		iter.old_balance,
		iter.new_balance,
		when
	    );
	}

	printf("%s\n", ft_to_string(table));
	ft_destroy_table(table);
    }

    printf("--- View Details ---\n");
}

void logout(int sd, id_t sid) {
    printf("--- Logout ---\n");

    Header head = {.action = Logout, .sid = sid};
    LogoutResponse resp;

    if (write(sd, &head, sizeof(head)) == -1) {
	syserr(AT);
	printf("Communication Error.\n");
    } else if (safe_read(sd, &resp, sizeof(resp)) == -1) {
	syserr(AT);
	printf("Communication Error.\n");
    } else if (resp.stat != Success) {
	printf("Request Failed.\n");
    } else {
	printf("Successfully logged out.\n");
    }

    if (close(sd) == -1) {
	syserr(AT);
    }

    printf("--- Logout ---\n");

    exit(0);
}

void password_change(int sd, id_t sid) {
    printf("--- Password Change ---\n");

    Header head = {.action = PasswordChange, .sid = sid};
    PasswordChangeRequest req;
    PasswordChangeResponse resp;

    printf("New Password: ");
    getchar(); // Newline from menu input
    fgets(req.password, INFO_STRING_LEN, stdin);
    req.password[strlen(req.password) - 1] = '\0';
    printf("\n");

    if (write(sd, &head, sizeof(head)) == -1) {
	syserr(AT);
	printf("Communication Error.\n");
    } else if (write(sd, &req, sizeof(req)) == -1) {
	syserr(AT);
	printf("Communication Error.\n");
    } else if (safe_read(sd, &resp, sizeof(resp)) == -1) {
	syserr(AT);
	printf("Communication Error.\n");
    } else if (resp.stat != Success) {
	printf("Request Failed.\n");
    } else {
	printf("Password changed successfully.\n");
    }

    printf("--- Password Change ---\n");
}

void menu(id_t sid) {
	char *m = "--- Menu ---\n"
		"\t1) Withdraw\n"
		"\t2) Deposit\n"
		"\t3) Balance Enquiry\n"
		"\t4) View Details\n"
		"\t5) Password Change\n"
		"\t6) Logout and Exit\n"
		"--- Menu ---\n";

	int opt;
	do {
		printf("%s\nYour choice: ", m);
		scanf("%d", &opt);
		system("clear");

		int sd = connect_to_server(PORTNO);
		if (sd == EFAIL) {
			apperr(AT, "Could not connect to server");
			return;
		}

		switch(opt) {
			case 1:
				withdraw(sd, sid);
				break;

			case 2:
				deposit(sd, sid);
				break;

			case 3:
				balance_enquiry(sd, sid);
				break;

			case 4:
				view_details(sd, sid);
				break;

			case 5:
				password_change(sd, sid);
				break;

			case 6:
				logout(sd, sid);
				break;

			default:
				printf("Invalid choice\n");
		}

		wait_for_enter();
		system("clear");

		if (close(sd) == -1) {
			syserr(AT);
		}
	} while(1);
}

int main() {
	int sd = connect_to_server(PORTNO);
	if (sd == -1) {
		printf("Cannot connect to server\n");
		return 1;
	}

	LoginResponse lresp;
	if (login(sd, &lresp) == EFAIL) {
		if (close(sd) == -1) {
			syserr(AT);
		}
		return -1;
	}

	if (close(sd) == -1) {
		syserr(AT);
	}
	printf("\n");

	menu(lresp.sid);
}
