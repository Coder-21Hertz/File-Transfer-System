/*
 * @Description: 服务器程序
 * @Company: SZU
 * @Author: PerkyRookie
 * @Date: 2019-04-08 14:19:59
 * @LastEditors: PerkyRookie
 * @LastEditTime: 2019-07-19 14:22:32
 */

#include "login.h"
#include "common.h"
#include "pthread_pool.h"

SSL_CTX *ctx;
struct sockaddr_in clientaddr;

void *myprocess(int args)
{
	int connfd = args;
	char claddrStr[INET_ADDRSTRLEN];
	/* 获取客户端ip地址 */
	inet_ntop(AF_INET, &clientaddr.sin_addr, claddrStr, INET_ADDRSTRLEN);

	/* 基于 ctx 产生一个新的 SSL */
	SSL *ssl = SSL_new(ctx);
	/* 将连接用户的 socket 加入到 SSL */
	SSL_set_fd(ssl, connfd);
	/* 建立 SSL 连接 */
	if (SSL_accept(ssl) == -1)
		errExit("myprocess.SSL_accept");

	char buff[BUFFSIZE];
	if(server_login(ssl) == 'q')
	{
		/* 关闭 SSL 连接 */
		SSL_shutdown(ssl);
		/* 释放 SSL */
		SSL_free(ssl);
		close(connfd);
		return NULL;
	}
	while(1)
	{
		memset(buff, 0, BUFFSIZE);

		/* 读取命令行输入信息 */
		int len=SSL_read(ssl,buff,BUFFSIZE);
		if(len <= 0)
			errExit("myprocess.SSL_read.1");

		if (*buff == 'q')
			break;
		/* 分割命令行输入信息 */
		char filename[FILE_NAME_MAX_SIZE];
		char operate = my_strtok(buff, filename);

		/* 发送文件到客户端 */
		if(operate == 'r')
		{
			send_file(filename, ssl, claddrStr);
		}
		/* 从客户端读取文件 */
		else if (operate == 's')
		{
			recv_file(filename, ssl, claddrStr);
		}
	}

	/* 关闭 SSL 连接 */
	SSL_shutdown(ssl);
	/* 释放 SSL */
	SSL_free(ssl);
	close(connfd);

	return NULL;
}


int main(int argc, char *argv[])
{
	// char sql[100];

	if (argc != 3 || strcmp(argv[1], "--help") == 0)
		usageErr("%s cacert.pem privkey.pem\n", argv[0]);
	setbuf(stdout,NULL);	//关闭缓冲

	//初始化线程池
	pool_init(5);

	/* SSL 库初始化 */
	SSL_library_init();
	/* 载入所有 SSL 算法 */
	OpenSSL_add_all_algorithms();
	/* 载入所有 SSL 错误消息 */
	SSL_load_error_strings();
	/* 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX ，即 SSL Content Text */
	ctx = SSL_CTX_new(SSLv23_server_method());
	/* 也可以用 SSLv2_server_method() 或 SSLv3_server_method() 单独表示 V2 或 V3标准 */
	if (ctx == NULL)
		errExit("SSL_CTX_new");
	/* 载入用户的数字证书， 此证书用来发送给客户端。 证书里包含有公钥 */
	if (SSL_CTX_use_certificate_file(ctx, argv[1], SSL_FILETYPE_PEM) <= 0)
		errExit("SSL_CTX_use_certificate_file");
	/* 载入用户私钥 */
	if (SSL_CTX_use_PrivateKey_file(ctx, argv[2], SSL_FILETYPE_PEM) <= 0)
		errExit("SSL_CTX_use_PrivateKey_file");
	/* 检查用户私钥是否正确 */
	if (!SSL_CTX_check_private_key(ctx))
		errExit("SSL_CTX_check_private_key");

	/* 创建 socket */
	int sockfd = socket_init();
	if(sockfd == -1)
		errExit("socket_init");
	printf("服务器启动成功！\n");

	int connfd;

	while (1)
	{
		socklen_t length = sizeof(clientaddr);
		connfd = accept(sockfd, (struct sockaddr *)&clientaddr, &length);
		if (connfd == -1)
			errExit("accept");
		if (getpeername(connfd, (struct sockaddr *)&clientaddr, &length) == -1)
			errExit("getpeername");

		//给线程池添加任务
		pool_add_task(myprocess,connfd);
	}
	close(sockfd);
	SSL_CTX_free(ctx);
	return 0;
}