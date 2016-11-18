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
	int  slen = sizeof(si_other);
	char buf[BUFLEN];

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		on_err(strerror(errno);
	struct sockaddr_in si_other;
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	if (inet_aton(SRV_IP, &si_other.sin_addr) == 0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	for (int i = 0; i < NPACK; i++)
	{
		printf("Sending packet %d\n", i);
		sprintf(buf, "This is packet %d\n", i);
		if (sendto(s, buf, BUFLEN, 0, &si_other, slen) == -1)
			on_err(strerror(errno);
	}

	close(s);
	return 0;
}
