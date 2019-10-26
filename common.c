/*
 * @Description: 服务器和客户端共用头文件
 * @Company: SZU
 * @Author: PerkyRookie
 * @Date: 2019-04-09 09:22:49
 * @LastEditors: PerkyRookie
 * @LastEditTime: 2019-07-23 14:57:57
 * @Compilation: gcc server.c common.c pthread_pool.c login.c -o server -ltlpi -lssl -lcrypto -ldl -lpthread -I/usr/include/mysql/ -lmysqlclient -Wall -g
 * @Compilation: gcc client.c common.c pthread_pool.c login.c -o client -ltlpi -lssl -lcrypto -ldl -lpthread -I/usr/include/mysql/ -lmysqlclient -Wall -g
 * @running: ./server cacert.pem privkey.pem   ./client ip地址
 */

#include "common.h"

/*
 * @description: 用于分割命令行输入的操作命令，分隔符为"+"
 * @param {char *} buff: 要分割的字符串
 * @param {char []} filename: 分割后的第二个字符串
 * @return: 返回命令行输入第一个字符
 */
char my_strtok(char *buff, char filename[FILE_NAME_MAX_SIZE])
{
    const char separator[2] = "+";
    memset(filename, 0, FILE_NAME_MAX_SIZE);
    /* 获取第一个子字符串(判断发送还是接收) */
    char *operate = strtok(buff, separator);
    /* 获取第二个子字符串(文件名) */
    char *filen = strtok(NULL,separator);
    /*限制文件名长度 */
    strncpy(filename, filen, strlen(filen)>FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE
                                                              : strlen(filen));
    return *operate;
}

/*
 * @description: 发送一个文件
 * @param {char *} filename: 要发送的文件
 * @param {SSL *} ssl: 建立的ssl连接
 * @return: 成功则返回0，出错返回-1
 */
void send_file(char *filename, SSL *ssl, char *claddrStr)
{
    FILE *fd=fopen(filename, "r");
    socklen_t len;
    if(fd == NULL)
    {
        printf("File :%s not found!\n", filename);
        /* 通知客户端，没有这个文件 */
        len = SSL_write(ssl, "not", strlen("not"));
        if (len <= 0)
            errExit("send_file.SSL_write.1");
    }
    else
    {
        len = SSL_write(ssl, "yes", strlen("yes"));
        if (len <= 0)
            errExit("send_file.SSL_write.2");

        /* 读取并发送文件长度 */
        struct stat fstat;
        if((stat(filename,&fstat)) == -1)
            errExit("send_file.stat");
        len = SSL_write(ssl,(void *)&fstat.st_size,4);
        if (len <= 0)
            errExit("send_file.SSL_write.3");

        printf("准备发送文件: %s 到 %s \n", filename, claddrStr);
        char buff[BUFFSIZE];
        memset(buff, 0, BUFFSIZE);
        int file_block_length=0;
        while((file_block_length = fread(buff, sizeof(char), BUFFSIZE, fd))>0)
        {
            printf("file_block_length:%d\n",file_block_length);
            if(SSL_write(ssl, buff, file_block_length) == -1)
                errExit("send_file.SSL_write.4");

            memset(buff, 0, BUFFSIZE);
        }
        printf("发送完成!\n");
        fclose(fd);
    }
}

/*
 * @description: 读取一个文件
 * @param {char *} filename: 要读取的文件
 * @param {SSL *} ssl: 建立的ssl连接
 * @return: 成功则返回0，出错返回-1
 */
void recv_file(char *filename, SSL *ssl, char *claddrStr)
{
    int length=0;
    char buff[BUFFSIZE];
    memset(buff, 0, BUFFSIZE);
    /* 判断文件是否存在 */
    length=SSL_read(ssl,buff,BUFFSIZE);
    if(length <= 0)
        errExit("recv_file.SSL_read.1");
    if(*buff == 'n')
    {
        printf("File :%s not found!\n", filename);
    }
    else
    {
        FILE *fd = fopen(filename,"wb+");  //打开或者创建一个文件
        if(fd==NULL)
            errExit("recv_file.fopen");

        /* 开始接收文件 */
        char buff[BUFFSIZE];
        memset(buff, 0, BUFFSIZE);

        int filesize=0;
        int totalrecv=0;
        /* 接收文件长度 */
        length = SSL_read(ssl, &filesize, 4);
        if(length <= 0)
            errExit("recv_file.SSL_read.2");

        while((length = SSL_read(ssl,buff,BUFFSIZE)))
        {
            if(length <= 0)
                errExit("recv_file.SSL_read.3");
            if(fwrite(buff, sizeof(char), length, fd) < length)
                errExit("recv_file.fwrite");
            /* 匹配文件长度 */
            totalrecv += length;
            if(totalrecv == filesize)
                break;

            memset(buff, 0, BUFFSIZE);
        }
        printf("收到文件: %s 来自 %s !\n", filename, claddrStr);
        fclose(fd);
    }
}

/*
 * @description: socket连接服务端初始化
 * @param {type} 无
 * @return: 成功则返回0，出错返回-1
 */
int socket_init()
{
    //创建 socket
	struct sockaddr_in svraddr;
	memset(&svraddr, 0, sizeof(svraddr));

	svraddr.sin_family = AF_INET;
	svraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	svraddr.sin_port = htons(PORT);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		errExit("socket_init.socket");

    /* 防止重启进入TIME_WAIT状态 */
	int reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
		errExit("socket_init.setsockopt");

	if (bind(sockfd, (struct sockaddr *)&svraddr, sizeof(svraddr)) == -1)
		errExit("socket_init.bind");

	if (listen(sockfd, LISTENQ) == -1)
		errExit("socket_init.listen");

    return sockfd;
}


/*
 * @description: 输出证书信息
 * @param {SSL *} ssl: 建立的ssl连接
 * @return: 无
 */
void ShowCerts (SSL* ssl)
{
    X509 *cert;
    char *line;

    cert=SSL_get_peer_certificate(ssl);
    if(cert != NULL){
        printf("数字证书信息：\n");
        line=X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("证书：%s\n", line);
        free(line);
        line=X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("颁发者：%s\n", line);
        free(line);
        X509_free(cert);
    }else
        printf("无证书信息！\n");
}