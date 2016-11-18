#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define BUFLEN 512
#define NPACK 10
#define PORT 9930

void on_err(char *s)
{
	perror(s);
	exit(1);
}

int main(void)
{	
	int s = -1;
	int slen = sizeof(si_other);
	char buf[BUFLEN];

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		on_err(strerror(errno));

	struct sockaddr_in si_me, si_other;
	memset((char *)&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, &si_me, sizeof(si_me)) == -1)
		on_err(strerror(errno));

	for (int i = 0; i < NPACK; i++)
	{
		if (recvfrom(s, buf, BUFLEN, 0, &si_other, &slen) == -1)
			on_err(strerror(errno));
		printf("Received packet from %s:%d\nData: %s\n\n",
			inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

	}
	close(s);
	return 0;
}
