/*
 * @Description: 账号管理系统
 * @Company: SZU
 * @Author: PerkyRookie
 * @Date: 2019-04-10 14:34:26
 * @LastEditors: PerkyRookie
 * @LastEditTime: 2019-07-11 20:46:20
 */

#include "login.h"

int callback(void *NotUsed, int argc, char *argv[], char **azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        strcpy(passwd_d, argv[i]);
    }
    return 0;
}

char server_login(SSL *ssl)
{
    char username[20];
    char password[20];
    int login_or_create;
    char temp[100];
    char buf[10];
    int login_flag = 0;

    /* 检测是否退出 */
    if(SSL_read(ssl, temp, 100) <= 0)
        errExit("server_login.SSL_read.1");
    if(*temp == 'q')
        return *temp;

    MYSQL *conn_ptr;
    MYSQL_RES *res_ptr;
    MYSQL_ROW sqlrow;
    int res;
    char sql[100];

	conn_ptr = mysql_init(NULL);
	if (!conn_ptr) {
		printf("mysql_init failed\n");
		return EXIT_FAILURE;
	}
    //root 为用户名 123456为密码 project_ssl_login为要连接的database
	conn_ptr = mysql_real_connect(conn_ptr, "localhost", "root", "123456", "project_ssl_login", 0, NULL, 0);

    do{
        login_flag = 1;
        if(SSL_read(ssl, temp, 100) <= 0)
            errExit("server_login.SSL_read.2");

        sscanf(temp, "log:%d username:%s password:%s", &login_or_create, username, password);

        /* 注册 */
        if(login_or_create == 2)
        {
            sprintf(sql, "insert into login values('%s','%s')",username, password);

            res = mysql_query(conn_ptr, sql);

            if(res)
            {
                printf("用户名存在：username:%s \n",passwd_d);
                login_flag = 3;
            }
            else
            {
                printf("用户注册：username:%s password:%s\n",username,password);
                login_flag = 2;
            }
        }
        //登录
        else
        {
            sprintf(sql, "select password from login where username='%s';",username);
            res = mysql_query(conn_ptr, sql);
            if (res) {
                printf("SELECT error:%s\n",mysql_error(conn_ptr));
            } else {
                res_ptr = mysql_store_result(conn_ptr);				//取出结果集
                if(res_ptr) {
                    sqlrow = mysql_fetch_row(res_ptr);
                    printf("%s\n", sqlrow[0]);
                    if(strcmp(password,sqlrow[0]) == 0)
                    {
                        printf("用户登录：username:%s password:%s\n",username,passwd_d);
                        login_flag = 1;
                    }
                    else
                    {
                        printf("登录失败：username:%s password:%s\n",username,passwd_d);
                        login_flag = 0;
                    }
                }
            }
        }
        sprintf(buf,"login:%d",login_flag);

	    if(SSL_write(ssl,buf,10) <= 0)
            errExit("server_login.SSL_write.1");

    }while(login_flag == 0 || login_flag == 3);

	mysql_close(conn_ptr);

	return 's';
}

int client_login(SSL *ssl)
{
    int login_or_create;
    int login_flag = 0;
    char username[20];
    char password[20];
    char temp[100];
    char buf[10];
    do
    {
        memset(buf, 0, 10);
        do{
            printf("请输入您的选择(1: 登录，2: 注册，3: 退出): ");
            scanf("%d", &login_or_create);
            getchar();
        }while(login_or_create != 1 && login_or_create != 2 && login_or_create != 3);


        if(login_or_create == 3){
            if(SSL_write(ssl, "quit", 100) <= 0)
                errExit("client_login.SSL_write.1");
            return login_or_create;
        }
        if(SSL_write(ssl, "continue", 100) <= 0)
            errExit("client_login.SSL_write.2");

        printf("用户名: ");
        scanf("%s", username);
        getchar();
        printf("密码: ");
        scanf("%s", password);
        getchar();
        sprintf(temp, "log:%d username:%s password:%s", login_or_create, username, password);

        if(SSL_write(ssl, temp, 100) <= 0)
            errExit("client_login.SSL_write.3");
        if(SSL_read(ssl, buf, 10) <= 0)
            errExit("client_login.SSL_read.1");
        sscanf(buf, "login:%d", &login_flag);

        if (login_flag == 1)
        {
            printf("---------登录成功!---------\n");
        }
        else if (login_flag == 2)
        {
            printf("---------注册成功!---------\n");
        }
        else if(login_flag == 3)
        {
            printf("--------用户名已存在!-------\n");
        }
        else if(login_flag == 0)
        {
            printf("------用户名或密码错误!------\n");
        }
    } while (login_flag == 0 || login_flag == 3);

    return login_or_create;
}