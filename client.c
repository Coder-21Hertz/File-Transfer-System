/*
 * @Description: 客户端程序
 * @Company: SZU
 * @Author: PerkyRookie
 * @Date: 2019-04-08 14:31:56
 * @LastEditors: PerkyRookie
 * @LastEditTime: 2019-07-11 16:07:31
 */

#include "login.h"
#include "common.h"


int main(int argc, char *argv[])
{
    if(argc != 2 || strcmp(argv[1], "--help") == 0)
		usageErr("%s IP_Address\n", argv[0]);
    setbuf(stdout,NULL);	//关闭缓冲

	SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx=SSL_CTX_new(SSLv23_client_method());
    if(ctx==NULL)
		errExit("SSL_CTX_new");

    int clientfd=socket(AF_INET,SOCK_STREAM,0);
    if(clientfd == -1)
		errExit("socket");

    /* IPv4的socket 地址 */
    struct sockaddr_in svraddr;
    memset(&svraddr, 0, sizeof(svraddr));
    /* 取出ip地址 */
    if(inet_pton(AF_INET, argv[1], &svraddr.sin_addr)==0)
        errExit("inet_pton");
    svraddr.sin_family=AF_INET;
    svraddr.sin_port=htons(PORT);

    if(connect(clientfd, (struct sockaddr*)&svraddr, sizeof(svraddr)) == -1)
		errExit("connect");

    SSL *ssl=SSL_new(ctx);
    SSL_set_fd(ssl,clientfd);
    if(SSL_connect(ssl) == -1)
        errExit("SSL_connect");
    /* 输出证书信息 */
    ShowCerts(ssl);

    /* 登录操作 */
    if(client_login(ssl) == 3)
    {
        close(clientfd);
        SSL_CTX_free(ctx);
        return 0;
    }

    char cmd[BUFFSIZE];
    while(1)
    {
        memset(cmd, 0, BUFFSIZE);
        printf("请输入您要操作和文件(recv/send+file): ");
        scanf("%s",cmd);
        getchar();  //去除末尾回车符

        /* 其他字符，不发送 */
        if(*cmd != 'r' && *cmd != 's' && *cmd != 'q')
        {
            printf("无效的输入命令\n");
            continue;
        }

        /* 发送命令行输入信息 */
        socklen_t len = SSL_write(ssl, cmd, strlen(cmd));
        if (len <= 0)
            errExit("SSL_write");

        if(*cmd == 'q')
        {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            break;
        }

        /* 分割命令行输入信息 */
        char filename[FILE_NAME_MAX_SIZE];
        char operate = my_strtok(cmd, filename);

        /* 从服务器读取文件 */
        if(operate == 'r')
        {
            recv_file(filename, ssl, argv[1]);
        }
        /* 发送文件到服务器 */
        else if(operate == 's')
        {
            send_file(filename, ssl, argv[1]);
        }

    }
    close(clientfd);
    SSL_CTX_free(ctx);
    return 0;
}