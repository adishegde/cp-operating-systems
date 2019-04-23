#ifndef _DATABASE_H
#define _DATABASE_H

#include <time.h>
#include <pthread.h>

#include "commons.h"

#define DB_BASE_PATH "./db"
#define DB_USER_PATH "./db/user.dat"
#define DB_ACCOUNT_PATH "./db/account.dat"
#define DB_TRANS_PATH "./db/transaction.dat"

#define id_t unsigned long int  // Data type for ids
#define balance_t double    // Data type for balance

/* BEGIN - Models */
typedef struct UserModel {
    char name[INFO_STRING_LEN];
    char uname[INFO_STRING_LEN];
    char password[INFO_STRING_LEN];
    int is_admin;
    id_t aid;
    id_t id;
} UserModel;

typedef struct AccountModel {
    id_t id;
    balance_t balance;
} AccountModel;

typedef struct TransactionModel {
    id_t aid;
    id_t uid;
    id_t id;
    time_t when;
    balance_t old_balance;
    balance_t new_balance;
} TransactionModel;
/* END - Models */

/* BEGIN - Functions */
int db_init();

int create_user(UserModel *user);
int update_user(UserModel *user);
int delete_user(id_t uid);
int get_num_users();
int get_users(UserModel *users, int limit);
int get_user_from_id(id_t uid, UserModel *user);
int get_user_from_uname(char *uname, UserModel *user);

int create_account(AccountModel *account);
int update_account(AccountModel *account);
int delete_account(id_t aid);
int get_num_accounts();
int get_accounts(AccountModel *accounts, int limit);
int get_account_from_id(id_t aid, AccountModel *account);

int append_transaction(TransactionModel *trans);
int get_num_transactions(id_t aid);
int get_transactions(id_t aid, TransactionModel *trans, int limit);
/* END - Functions */
#endif
