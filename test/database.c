#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include "../database.h"

void print_user(UserModel u) {
	printf("ID: %lu Name: %s Acc: %lu\n", u.id, u.name, u.aid);
}

int main() {
	if (db_init() < 0) {
		printf("db_init: Failed\n");
		return 1;
	}
	printf("db_init: Successful\n");
	printf("\n\n");

	char *usernames[] = {"test1", "test2", "test3"};
	id_t accountIds[3];

	AccountModel acc = {.balance = 1000.5};

	int i;
	for(i = 0; i < 3; ++i) {
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
	printf("get_users: Successful\n");
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
}
