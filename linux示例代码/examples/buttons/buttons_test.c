#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

int main(void)
{
	int i;
	int buttons_fd;
	int key_value[4];

	/*�򿪼����豸�ļ�*/
	buttons_fd = open("/dev/buttons", 0);
	if (buttons_fd < 0) {
		perror("open device buttons");
		exit(1);
	}

	for (;;) {
		fd_set rds;
		int ret;

		FD_ZERO(&rds);
		FD_SET(buttons_fd, &rds);

		/*ʹ��ϵͳ����select����Ƿ��ܹ���/dev/buttons�豸��ȡ����*/
		ret = select(buttons_fd + 1, &rds, NULL, NULL, NULL);
		
		/*��ȡ�������˳�����*/
		if (ret < 0) {
			perror("select");
			exit(1);
		}
		
		if (ret == 0) {
			printf("Timeout.\n");
		} 
		/*�ܹ���ȡ������*/
		else if (FD_ISSET(buttons_fd, &rds)) {
			/*��ʼ��ȡ�����������������ݣ�ע��key_value�ͼ��������ж���Ϊһ�µ�����*/
			int ret = read(buttons_fd, key_value, sizeof key_value);
			if (ret != sizeof key_value) {
				if (errno != EAGAIN)
					perror("read buttons\n");
				continue;
			} else {
				/*��ӡ��ֵ*/
				for (i = 0; i < 4; i++)
				    printf("K%d %s, key value = 0x%02x\n", i, (key_value[i] & 0x80) ? "released"     : \
				    	                                                 key_value[i] ? "pressed down" : "", \
				    	                                         key_value[i]);
			}
				
		}
	}

	/*�ر��豸�ļ����*/
	close(buttons_fd);
	return 0;
}
