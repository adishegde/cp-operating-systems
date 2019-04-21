#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include "../database.h"

void print_user(UserModel u) {
	printf("ID: %lu Name: %s Acc: %lu\n", u.id, u.name, u.aid);
}

void print_account(AccountModel a) {
	printf("ID: %lu Balance: %f\n", a.id, a.balance);
}

int test_account_functions(id_t *accountIds) {
	AccountModel acc;
	acc.balance = 1000.5;

	int i;
	for(i = 0; i < 4; ++i) {
		if (create_account(&acc) < 0) {
			printf("create_account: Failed\n");
			return 1;
		} else {
			accountIds[i] = acc.id;
			printf("Account created with ID = %lu\n", acc.id);
		}
	}
	printf("create_account: Successful\n");
	printf("\n\n");

	int size = get_num_accounts();
	printf("Number of accounts: %d\n", size);
	printf("get_num_accounts: Successful\n");
	printf("\n\n");

	AccountModel *ua = malloc(size * sizeof(AccountModel));
	if (get_accounts(ua, size) < 0) {
		printf("get_accounts: Failed\n");
		return 1;
	}
	for(i = 0; i < size; ++i) {
		print_account(ua[i]);
	}
	printf("get_accounts: Successful\n");
	printf("\n\n");

	if(get_account_from_id(ua[1].id, &acc) < 0) {
		printf("get_account_from_id: Failed");
		return 1;
	}
	printf("Requested: ");
	print_account(ua[1]);
	printf("Returned: ");
	print_account(acc);
	printf("get_account_from_id: Successful\n");
	printf("\n\n");

	if (delete_account(ua[3].id) < 0) {
		printf("delete_account: Failed\n");
		return 1;
	}
	if(get_account_from_id(ua[3].id, &acc) != ENOTFOUND) {
		printf("get_account_from_id: Failed");
		return 1;
	}
	if (delete_account(-100) >= 0) {
		printf("delete_account: Failed\n");
		return 1;
	}
	printf("delete_account: Successful\n");
	printf("\n\n");

	acc = ua[2];
	acc.balance *= 99;
	if (update_account(&acc) < 0) {
		printf("update_account: Failed\n");
		return 1;
	}
	acc.id = -100;
	if (update_account(&acc) != ENOTFOUND) {
		printf("update_account: Failed\n");
		return 1;
	}
	if (get_account_from_id(ua[2].id, &acc) < 0) {
		printf("get_account_from_id: Failed\n");
		return 1;
	}
	printf("Old Value: ");
	print_account(ua[2]);
	printf("Updated Value: ");
	print_account(acc);
	printf("update_account: Successful\n");
	printf("\n\n");

	free(ua);

	return 0;
}

int test_user_functions(id_t *accountIds) {
	char *usernames[] = {"test1", "test2", "test3"};

	int i;
	UserModel u;

	for(i = 0; i < 3; ++i) {
		strcpy(u.name, usernames[i]);
		strcpy(u.username, usernames[i]);
		strcpy(u.password, usernames[i]);
		u.aid = accountIds[i];
		u.is_admin = i%2;

		if (create_user(&u) < 0) {
			printf("create_user: Failed");
			return 1;
		} else {
			printf("User created with ID = %lu\n", u.id);
		}
	}
	printf("create_user: Successful\n");
	printf("\n\n");

	int size = get_num_users();
	printf("Number of users: %d\n", size);
	printf("get_num_users: Successful\n");
	printf("\n\n");

	UserModel *ua = malloc(size * sizeof(UserModel));
	if (get_users(ua, size) < 0) {
		printf("get_users: Failed\n");
		return 1;
	}
	for(i = 0; i < size; ++i) {
		print_user(ua[i]);
	}
	printf("get_users: Successful\n");
	printf("\n\n");

	if(get_user_from_id(ua[1].id, &u) < 0) {
		printf("get_user_from_id: Failed");
		return 1;
	}
	printf("Requested: ");
	print_user(ua[1]);
	printf("Returned: ");
	print_user(u);
	printf("get_user_from_id: Successful\n");
	printf("\n\n");

	if (delete_user(ua[2].id) < 0) {
		printf("delete_user: Failed\n");
		return 1;
	}
	if(get_user_from_id(ua[2].id, &u) != ENOTFOUND) {
		printf("get_user_from_id: Failed");
		return 1;
	}
	if (delete_user(-100) >= 0) {
		printf("delete_user: Failed\n");
		return 1;
	}
	printf("delete_user: Successful\n");
	printf("\n\n");

	u = ua[1];
	strcpy(u.name, "updated_user");
	if (update_user(&u) < 0) {
		printf("update_user: Failed\n");
		return 1;
	}
	u.id = -100;
	if (update_user(&u) != ENOTFOUND) {
		printf("update_user: Failed\n");
		return 1;
	}
	if (get_user_from_id(ua[1].id, &u) < 0) {
		printf("get_user_from_id: Failed\n");
		return 1;
	}
	printf("Old Value: ");
	print_user(ua[1]);
	printf("Updated Value: ");
	print_user(u);
	printf("update_user: Successful\n");
	printf("\n\n");

	free(ua);

	return 0;
}

int main() {
	if (db_init() < 0) {
		printf("db_init: Failed\n");
		return 1;
	}
	printf("db_init: Successful\n");
	printf("\n\n");

	id_t accountIds[4];

	if (test_account_functions(accountIds) != 0) {
		return 1;
	}

	if (test_user_functions(accountIds) != 0) {
		return 1;
	}

	return 0;
}
