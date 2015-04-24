/*
	Copyright (C) 2013 - 2014 CurlyMo

	This file is part of pilight.

	pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#ifndef __FreeBSD__
	#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#ifdef _WIN32
	#if _WIN32_WINNT < 0x0501
		#undef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#include <winsock2.h>
	#include <windows.h>
	#include <psapi.h>
	#include <tlhelp32.h>
	#include <ws2tcpip.h>
	#include <iphlpapi.h>
#else
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <sys/wait.h>
	#include <net/if.h>
	#include <ifaddrs.h>
	#include <pwd.h>
	#include <sys/ioctl.h>
#endif
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifndef __USE_XOPEN
	#define __USE_XOPEN
#endif
#include <sys/time.h>
#include <time.h>

#include "../config/settings.h"
#include "mem.h"
#include "common.h"
#include "log.h"

char *progname;

static unsigned int ***whitelist_cache = NULL;
static unsigned int whitelist_number;
static const char base64table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};
static pthread_mutex_t atomic_lock;
static pthread_mutexattr_t atomic_attr;

void atomicinit(void) {
	pthread_mutexattr_init(&atomic_attr);
	pthread_mutexattr_settype(&atomic_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&atomic_lock, &atomic_attr);
}

void atomiclock(void) {
	pthread_mutex_lock(&atomic_lock);
}

void atomicunlock(void) {
	pthread_mutex_unlock(&atomic_lock);
}

int inetdevs(char ***array) {
	unsigned int nrdevs = 0, i = 0, match = 0;

#ifdef _WIN32
	IP_ADAPTER_INFO *pAdapter = NULL;
	ULONG buflen = sizeof(IP_ADAPTER_INFO);
	IP_ADAPTER_INFO *pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(buflen);

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = MALLOC(buflen);
	}

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
		for(pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
			match = 0;

			for(i=0;i<nrdevs;i++) {
				if(strcmp((*array)[i], pAdapter->AdapterName) == 0) {
					match = 1;
					break;
				}
			}
			if(match == 0 && strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0) {
				if((*array = REALLOC(*array, sizeof(char *)*(nrdevs+1))) == NULL) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				if(((*array)[nrdevs] = MALLOC(strlen(pAdapter->AdapterName)+1)) == NULL) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				strcpy((*array)[nrdevs], pAdapter->AdapterName);
				nrdevs++;
			}
		}
	}
	if(pAdapterInfo != NULL) {
		FREE(pAdapterInfo);
	}
#else
	int family = 0, s = 0;
	char host[NI_MAXHOST];
	struct ifaddrs *ifaddr, *ifa;

#ifdef __FreeBSD__
	if(rep_getifaddrs(&ifaddr) == -1) {
		logprintf(LOG_ERR, "could not get network adapter information");
		exit(EXIT_FAILURE);
	}
#else
	if(getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
#endif

	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if(ifa->ifa_addr == NULL) {
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if((strstr(ifa->ifa_name, "lo") == NULL && strstr(ifa->ifa_name, "vbox") == NULL
		    && strstr(ifa->ifa_name, "dummy") == NULL) && (family == AF_INET || family == AF_INET6)) {
			memset(host, '\0', NI_MAXHOST);

			s = getnameinfo(ifa->ifa_addr,
                           (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                 sizeof(struct sockaddr_in6),
                           host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if(s != 0) {
				logprintf(LOG_ERR, "getnameinfo() failed: %s", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			if(strlen(host) > 0) {
				match = 0;
				for(i=0;i<nrdevs;i++) {
					if(strcmp((*array)[i], ifa->ifa_name) == 0) {
						match = 1;
						break;
					}
				}
				if(match == 0) {
					if((*array = REALLOC(*array, sizeof(char *)*(nrdevs+1))) == NULL) {
						logprintf(LOG_ERR, "out of memory");
						exit(EXIT_FAILURE);
					}
					if(((*array)[nrdevs] = MALLOC(strlen(ifa->ifa_name)+1)) == NULL) {
						logprintf(LOG_ERR, "out of memory");
						exit(EXIT_FAILURE);
					}
					strcpy((*array)[nrdevs], ifa->ifa_name);
					nrdevs++;
				}
			}
		}
	}

#ifdef __FreeBSD__
	rep_freeifaddrs(ifaddr);
#else
	freeifaddrs(ifaddr);
#endif

#endif // _WIN32
	return (int)nrdevs;
}

#ifdef __FreeBSD__
int dev2ip(char *dev, char **ip, __sa_family_t type) {
#else
int dev2ip(char *dev, char **ip, sa_family_t type) {
#endif

#ifdef _WIN32
	IP_ADAPTER_INFO *pAdapter = NULL;
	ULONG buflen = sizeof(IP_ADAPTER_INFO);
	IP_ADAPTER_INFO *pAdapterInfo = MALLOC(buflen);

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = MALLOC(buflen);
	}

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
		for(pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
			if(strcmp(dev, pAdapter->AdapterName) == 0) {
				strcpy(*ip, pAdapter->IpAddressList.IpAddress.String);
				break;
			}
		}
	}
	if(pAdapterInfo != NULL) {
		FREE(pAdapterInfo);
	}
#else
	int fd = 0;
	struct ifreq ifr;

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
		logprintf(LOG_ERR, "could not create socket");
		return -1;
	}

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = type;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);

	if(ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
		close(fd);
		logprintf(LOG_ERR, "ioctl SIOCGIFADDR failed");
		return -1;

	}

	close(fd);

	struct sockaddr_in *ipaddr = (struct sockaddr_in *)(void *)&ifr.ifr_addr;
	inet_ntop(AF_INET, (void *)&(ipaddr->sin_addr), *ip, INET_ADDRSTRLEN+1);
#endif

	return 0;
}

unsigned int explode(char *str, const char *delimiter, char ***output) {
	if(str == NULL || output == NULL) {
		return 0;
	}
	unsigned int i = 0, n = 0, y = 0;
	size_t l = 0, p = 0;
	if(delimiter != NULL) {
		l = strlen(str);
		p = strlen(delimiter);
	}
	while(i < l) {
		if(strncmp(&str[i], delimiter, p) == 0) {
			if((i-y) > 0) {
				*output = REALLOC(*output, sizeof(char *)*(n+1));
				(*output)[n] = MALLOC((i-y)+1);
				strncpy((*output)[n], &str[y], i-y);
				(*output)[n][(i-y)] = '\0';
				n++;
			}
			y=i+p;
		}
		i++;
	}
	if(strlen(&str[y]) > 0) {
		*output = REALLOC(*output, sizeof(char *)*(n+1));
		(*output)[n] = MALLOC((i-y)+1);
		strncpy((*output)[n], &str[y], i-y);
		(*output)[n][(i-y)] = '\0';
		n++;
	}
	return n;
}

int host2ip(char *host, char *ip) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int rv = 0;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in *h = NULL;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((rv = getaddrinfo(host, NULL , NULL, &servinfo)) != 0) {
		logprintf(LOG_NOTICE, "getaddrinfo: %s, %s", host, gai_strerror(rv));
		return -1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		memcpy(&h, &p->ai_addr, sizeof(struct sockaddr_in *));
		memset(ip, '\0', INET_ADDRSTRLEN+1);
		inet_ntop(AF_INET, (void *)&(h->sin_addr), ip, INET_ADDRSTRLEN+1);
		if(strlen(ip) > 0) {
			freeaddrinfo(servinfo);
			return 0;
		}
	}

	freeaddrinfo(servinfo);
	return -1;
}

#ifdef _WIN32
int check_instances(const wchar_t *prog) {
	HANDLE m_hStartEvent = CreateEventW(NULL, FALSE, FALSE, prog);
	if(m_hStartEvent == NULL) {
		CloseHandle(m_hStartEvent);
		return 0;
	}

	if(GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(m_hStartEvent);
		m_hStartEvent = NULL;
		return 0;
	}
	return -1;
}

const char *inet_ntop(int af, const void *src, char *dst, int cnt) {
	struct sockaddr_in srcaddr;

	memset(&srcaddr, 0, sizeof(struct sockaddr_in));
	memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));

	srcaddr.sin_family = af;
	if(WSAAddressToString((struct sockaddr *)&srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD)&cnt) != 0) {
		DWORD rv = WSAGetLastError();
		printf("WSAAddressToString() : %d\n", (int)rv);
		return NULL;
	}
	return dst;
}

int setenv(const char *name, const char *value, int overwrite) {
	if(overwrite == 0) {
		value = getenv(name);
	}
	char c[strlen(name)+strlen(value)+1];
	strcat(c, name);
	strcat(c, "=");
	strcat(c, value);
	return putenv(c);
}

int unsetenv(const char *name) {
	char c[strlen(name)+1];
	strcat(c, name);
	strcat(c, "=");
	return putenv(c);
}

int inet_pton(int af, const char *src, void *dst) {
	struct sockaddr_storage ss;
	int size = sizeof(ss);
	char src_copy[INET6_ADDRSTRLEN+1];

	ZeroMemory(&ss, sizeof(ss));
	/* stupid non-const API */
	strncpy(src_copy, src, INET6_ADDRSTRLEN+1);
	src_copy[INET6_ADDRSTRLEN] = 0;

	if(WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
		switch(af) {
			case AF_INET:
				*(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
				return 1;
			case AF_INET6:
				*(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
				return 1;
			default:
				return 0;
		}
	}
	return 0;
}

int isrunning(const char *program) {
	DWORD aiPID[1000], iCb = 1000;
	DWORD iCbneeded = 0;
	int iNumProc = 0, i = 0;
	char szName[MAX_PATH];
	int iLenP = 0;
	HANDLE hProc;
	HMODULE hMod;

	iLenP = strlen(program);
	if(iLenP < 1 || iLenP > MAX_PATH)
		return -1;

	if(EnumProcesses(aiPID, iCb, &iCbneeded) <= 0) {
		return -1;
	}

	iNumProc = iCbneeded / sizeof(DWORD);

	for(i=0;i<iNumProc;i++) {
		strcpy(szName, "Unknown");
		hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aiPID[i]);

		if(hProc) {
			if(EnumProcessModules(hProc, &hMod, sizeof(hMod), &iCbneeded)) {
				GetModuleBaseName(hProc, hMod, szName, MAX_PATH);
			}
		}
		CloseHandle(hProc);

		if(strstr(szName, program) != NULL) {
			return aiPID[i];
		}
	}

	return -1;
}
#else
int isrunning(const char *program) {
	int pid = -1;
	char *tmp = MALLOC(strlen(program)+1);
	if(tmp == NULL) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	strcpy(tmp, program);
	if((pid = findproc(tmp, NULL, 1)) > 0) {
		FREE(tmp);
		return pid;
	}
	FREE(tmp);
	return -1;
}
#endif


#ifdef __FreeBSD__
int findproc(char *cmd, char *args, int loosely) {
#else
pid_t findproc(char *cmd, char *args, int loosely) {
#endif
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

#ifndef _WIN32
	DIR* dir;
	struct dirent* ent;
	char fname[512], cmdline[1024];
	int fd = 0, ptr = 0, match = 0, i = 0, y = '\n', x = 0;

	if(!(dir = opendir("/proc"))) {
	#ifdef __FreeBSD__
		system("mount -t procfs proc /proc");
		if(!(dir = opendir("/proc"))) {
			return -1;
		}
	#else
       	return -1;
	#endif
	}

    while((ent = readdir(dir)) != NULL) {
		if(isNumeric(ent->d_name) == 0) {
			sprintf(fname, "/proc/%s/cmdline", ent->d_name);
			if((fd = open(fname, O_RDONLY, 0)) > -1) {
				memset(cmdline, '\0', sizeof(cmdline));
				if((ptr = (int)read(fd, cmdline, sizeof(cmdline)-1)) > -1) {
					i = 0, match = 0, y = '\n';
					/* Replace all NULL terminators for newlines */
					for(i=0;i<ptr;i++) {
						if(i < ptr && cmdline[i] == '\0') {
							cmdline[i] = (char)y;
							y = ' ';
						}
					}
					cmdline[ptr] = '\0';
					match = 0;
					/* Check if program matches */
					char **array = NULL;
					unsigned int n = explode(cmdline, "\n", &array);

					if(n == 0) {
						close(fd);
						continue;
					}
					if((strcmp(array[0], cmd) == 0 && loosely == 0)
					   || (strstr(array[0], cmd) != NULL && loosely == 1)) {
						match++;
					}

					if(args != NULL && match == 1) {
						if(n <= 1) {
							close(fd);
							for(x=0;x<n;x++) {
								FREE(array[x]);
							}
							if(n > 0) {
								FREE(array);
							}
							continue;
						}
						if(strcmp(array[1], args) == 0) {
							match++;
						}

						if(match == 2) {
							pid_t pid = (pid_t)atol(ent->d_name);
							close(fd);
							closedir(dir);
							for(x=0;x<n;x++) {
								FREE(array[x]);
							}
							if(n > 0) {
								FREE(array);
							}
							return pid;
						}
					} else if(match > 0) {
						pid_t pid = (pid_t)atol(ent->d_name);
						close(fd);
						closedir(dir);
						for(x=0;x<n;x++) {
							FREE(array[x]);
						}
						if(n > 0) {
							FREE(array);
						}
						return pid;
					}
					for(x=0;x<n;x++) {
						FREE(array[x]);
					}
					if(n > 0) {
						FREE(array);
					}
				}
				close(fd);
			}
		}
	}
	closedir(dir);
#endif
	return -1;
}

int isNumeric(char *s) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	if(s == NULL || *s == '\0' || *s == ' ')
		return EXIT_FAILURE;
	char *p;
	strtod(s, &p);
	return (*p == '\0') ? EXIT_SUCCESS : EXIT_FAILURE;
}

int nrDecimals(char *s) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	unsigned int b = 0, c = strlen(s), i = 0;
	int a = 0;
	for(i=0;i<c;i++) {
		if(b == 1) {
			a++;
		}
		if(s[i] == '.') {
			b = 1;
		}
	}
	return a;
}

int name2uid(char const *name) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

#ifndef _WIN32
	if(name != NULL) {
		struct passwd *pwd = getpwnam(name); /* don't free, see getpwnam() for details */
		if(pwd) {
			return (int)pwd->pw_uid;
		}
	}
#endif
	return -1;
}


int which(const char *program) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char path[1024];
	strcpy(path, getenv("PATH"));
	char **array = NULL;
	unsigned int n = 0, i = 0;
	int found = -1;

	n = explode(path, ":", &array);
	for(i=0;i<n;i++) {
		char exec[strlen(array[i])+8];
		strcpy(exec, array[i]);
		strcat(exec, "/");
		strcat(exec, program);

		if(access(exec, X_OK) != -1) {
			found = 0;
			break;
		}
	}
	for(i=0;i<n;i++) {
		FREE(array[i]);
	}
	if(n > 0) {
		FREE(array);
	}
	return found;
}

int ishex(int x) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	return(x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F');
}

const char *rstrstr(const char* haystack, const char* needle) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char* loc = 0;
	char* found = 0;
	size_t pos = 0;

	while ((found = strstr(haystack + pos, needle)) != 0) {
		loc = found;
		pos = (size_t)((found - haystack) + 1);
	}

	return loc;
}

void alpha_random(char *s, const int len) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
	int i = 0;

	for(i = 0; i < len; ++i) {
			s[i] = alphanum[(unsigned int)rand() % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;
}

int urldecode(const char *s, char *dec) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *o;
	const char *end = s + strlen(s);
	int c;

	for(o = dec; s <= end; o++) {
		c = *s++;
		if(c == '+') {
			c = ' ';
		} else if(c == '%' && (!ishex(*s++) || !ishex(*s++)	|| !sscanf(s - 2, "%2x", &c))) {
			return -1;
		}
		if(dec) {
			sprintf(o, "%c", c);
		}
	}

	return (int)(o - dec);
}

static char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

char *urlencode(char *str) {
	char *pstr = str, *buf = MALLOC(strlen(str) * 3 + 1), *pbuf = buf;
	while(*pstr) {
		if(isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
			*pbuf++ = *pstr;
		else if(*pstr == ' ')
			*pbuf++ = '+';
		else
			*pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}

char *base64decode(char *src, size_t len, size_t *decsize) {
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int l = 0;
  size_t size = 0;
  char *dec = NULL;
  char buf[3];
  char tmp[4];

  dec = MALLOC(0);
  if(dec == NULL) {
		return NULL;
	}

  while(len--) {
    if('=' == src[j]) {
			break;
		}
    if(!(isalnum(src[j]) || src[j] == '+' || src[j] == '/')) {
			break;
		}

    tmp[i++] = src[j++];

    if(i == 4) {
      for(i = 0; i < 4; ++i) {
        for(l = 0; l < 64; ++l) {
          if(tmp[i] == base64table[l]) {
            tmp[i] = (char)l;
            break;
          }
        }
      }

      buf[0] = (char)((tmp[0] << 2) + ((tmp[1] & 0x30) >> 4));
      buf[1] = (char)(((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2));
      buf[2] = (char)(((tmp[2] & 0x3) << 6) + tmp[3]);

      dec = REALLOC(dec, size + 3);
      for(i = 0; i < 3; ++i) {
        dec[size++] = buf[i];
      }

      i = 0;
    }
  }

  if(i > 0) {
    for(j = i; j < 4; ++j) {
      tmp[j] = '\0';
    }

    for(j = 0; j < 4; ++j) {
			for(l = 0; l < 64; ++l) {
				if(tmp[j] == base64table[l]) {
					tmp[j] = (char)l;
					break;
				}
			}
    }

    buf[0] = (char)((tmp[0] << 2) + ((tmp[1] & 0x30) >> 4));
    buf[1] = (char)(((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2));
    buf[2] = (char)(((tmp[2] & 0x3) << 6) + tmp[3]);

    dec = REALLOC(dec, (size_t)(size + (size_t)(i - 1)));
    for(j = 0; (j < i - 1); ++j) {
      dec[size++] = buf[j];
    }
  }

  dec = REALLOC(dec, size + 1);
  dec[size] = '\0';

  if(decsize != NULL) {
		*decsize = size;
	}

  return dec;
}

char *base64encode(char *src, size_t len) {
  unsigned int i = 0;
  unsigned int j = 0;
  char *enc = NULL;
  size_t size = 0;
  char buf[4];
  char tmp[3];

  enc = MALLOC(0);
  if(enc == NULL) {
		return NULL;
	}

  while(len--) {
    tmp[i++] = *(src++);

    if(i == 3) {
      buf[0] = (char)((tmp[0] & 0xfc) >> 2);
      buf[1] = (char)(((tmp[0] & 0x03) << 4) + ((tmp[1] & 0xf0) >> 4));
      buf[2] = (char)(((tmp[1] & 0x0f) << 2) + ((tmp[2] & 0xc0) >> 6));
      buf[3] = (char)(tmp[2] & 0x3f);

      enc = REALLOC(enc, size + 4);
      for(i = 0; i < 4; ++i) {
        enc[size++] = base64table[(int)buf[i]];
      }

      i = 0;
    }
  }

  if(i > 0) {
    for(j = i; j < 3; ++j) {
      tmp[j] = '\0';
    }

		buf[0] = (char)((tmp[0] & 0xfc) >> 2);
		buf[1] = (char)(((tmp[0] & 0x03) << 4) + ((tmp[1] & 0xf0) >> 4));
		buf[2] = (char)(((tmp[1] & 0x0f) << 2) + ((tmp[2] & 0xc0) >> 6));
		buf[3] = (char)(tmp[2] & 0x3f);

    for(j = 0; (j < i + 1); ++j) {
      enc = REALLOC(enc, size+1);
      enc[size++] = base64table[(int)buf[j]];
    }

    while((i++ < 3)) {
      enc = REALLOC(enc, size+1);
      enc[size++] = '=';
    }
  }

  enc = REALLOC(enc, size+1);
  enc[size] = '\0';

  return enc;
}

void rmsubstr(char *s, const char *r) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	while((s=strstr(s, r))) {
		size_t l = strlen(r);
		memmove(s, s+l, 1+strlen(s+l));
	}
}

char *hostname(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char name[255] = {'\0'};
	char *host = NULL, **array = NULL;
	unsigned int n = 0, i = 0;

	gethostname(name, 254);
	if(strlen(name) > 0) {
		n = explode(name, ".", &array);
		if(n > 0) {
			if(!(host = MALLOC(strlen(array[0])+1))) {
				logprintf(LOG_ERR, "out of memory");
				exit(EXIT_FAILURE);
			}
			strcpy(host, array[0]);
		}
	}
	for(i=0;i<n;i++) {
		FREE(array[i]);
	}
	if(n > 0) {
		FREE(array);
	}
	return host;
}


char *distroname(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char dist[32];
	memset(dist, '\0', 32);
	char *distro = NULL;

#ifdef _WIN32
	strcpy(dist, "Windows");
#elif defined(__FreeBSD__)
	strcpy(dist, "FreeBSD/0.0");
#else
	int rc = 1;
	struct stat sb;
	if((rc = stat("/etc/redhat-release", &sb)) == 0) {
		strcpy(dist, "RedHat/0.0");
	} else if((rc = stat("/etc/SuSE-release", &sb)) == 0) {
		strcpy(dist, "SuSE/0.0");
	} else if((rc = stat("/etc/mandrake-release", &sb)) == 0) {
		strcpy(dist, "Mandrake/0.0");
	} else if((rc = stat("/etc/debian-release", &sb)) == 0) {
		strcpy(dist, "Debian/0.0");
	} else if((rc = stat("/etc/debian_version", &sb)) == 0) {
		strcpy(dist, "Debian/0.0");
	} else {
		strcpy(dist, "Unknown/0.0");
	}
#endif
	if(strlen(dist) > 0) {
		if(!(distro = MALLOC(strlen(dist)+1))) {
			logprintf(LOG_ERR, "out of memory");
			exit(EXIT_FAILURE);
		}
		strcpy(distro, dist);
		return distro;
	} else {
		return NULL;
	}
}

/* The UUID is either generated from the
   processor serial number or from the
   onboard LAN controller mac address */
char *genuuid(char *ifname) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *mac = NULL, *upnp_id = NULL;
	char serial[UUID_LENGTH+1];

	memset(serial, '\0', UUID_LENGTH+1);
#ifndef _WIN32
	char a[1024];
	FILE *fp = fopen("/proc/cpuinfo", "r");
	if(fp != NULL) {
		while(!feof(fp)) {
			if(fgets(a, 1024, fp) == 0) {
				break;
			}
			if(strstr(a, "Serial") != NULL) {
				sscanf(a, "Serial          : %16s%*[ \n\r]", (char *)&serial);
				if(strlen(serial) > 0 &&
					 ((isNumeric(serial) == EXIT_SUCCESS && atoi(serial) > 0) ||
					  (isNumeric(serial) == EXIT_FAILURE))) {
					memmove(&serial[5], &serial[4], 16);
					serial[4] = '-';
					memmove(&serial[8], &serial[7], 13);
					serial[7] = '-';
					memmove(&serial[11], &serial[10], 10);
					serial[10] = '-';
					memmove(&serial[14], &serial[13], 7);
					serial[13] = '-';
					upnp_id = MALLOC(UUID_LENGTH+1);
					strcpy(upnp_id, serial);
					fclose(fp);
					return upnp_id;
				}
			}
		}
		fclose(fp);
	}

#endif

#ifdef _WIN32
	int i = 0;

	if(ifname == NULL) {
		return NULL;
	}

	IP_ADAPTER_INFO *pAdapter = NULL;
	ULONG buflen = sizeof(IP_ADAPTER_INFO);
	IP_ADAPTER_INFO *pAdapterInfo = MALLOC(buflen);

	if((mac = MALLOC(13)) == NULL) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	memset(mac, '\0', 13);

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = MALLOC(buflen);
	}

	if(GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
		for(pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
			if(strcmp(ifname, pAdapter->AdapterName) == 0) {
				for(i = 0; i < pAdapter->AddressLength; i++) {
					sprintf(&mac[i*2], "%02x ", (unsigned char)pAdapter->Address[i]);
				}
				break;
			}
		}
	}

#elif defined(SIOCGIFHWADDR)
	int i = 0;
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(!(mac = MALLOC(13))) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	memset(mac, '\0', 13);
	struct ifreq s;

	memset(&s, '\0', sizeof(struct ifreq));
	strcpy(s.ifr_name, ifname);
	if(ioctl(fd, SIOCGIFHWADDR, &s) == 0) {
		for(i = 0; i < 12; i+=2) {
			sprintf(&mac[i], "%02x", (unsigned char)s.ifr_addr.sa_data[i/2]);
		}
	}
	close(fd);
#elif defined(SIOCGIFADDR)
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(!(mac = MALLOC(13))) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	memset(mac, '\0', 13);
	struct ifreq s;
	memset(&s.ifr_name, '\0', sizeof(s.ifr_name));
	memset(&s, '\0', sizeof(struct ifreq));
	strcpy(s.ifr_name, ifname);
	if(ioctl(fd, SIOCGIFADDR, &s) == 0) {
		int i;
		for(i = 0; i < 12; i+=2) {
			sprintf(&mac[i], "%02x", (unsigned char)s.ifr_addr.sa_data[i/2]);
		}
	}
	close(fd);
#elif defined(HAVE_GETIFADDRS)
	ifaddrs *iflist;
	if(getifaddrs(&iflist) == 0) {
		for(ifaddrs* cur = iflist; cur; cur = cur->ifa_next) {
			if((cur->ifa_addr->sa_family == AF_LINK) && (strcmp(cur->ifa_name, if_name) == 0) && cur->ifa_addr) {
				sockaddr_dl* sdl = (sockaddr_dl*)cur->ifa_addr;
				memcpy(&mac[i], LLADDR(sdl), sdl->sdl_alen);
				break;
			}
		}

		freeifaddrs(iflist);
	}
#endif

	if(strlen(mac) > 0) {
		upnp_id = MALLOC(UUID_LENGTH+1);
		memset(upnp_id, '\0', UUID_LENGTH+1);
		sprintf(upnp_id,
				"0000-%c%c-%c%c-%c%c-%c%c%c%c%c0",
				mac[0], mac[1], mac[2],
				mac[3], mac[4], mac[5],
				mac[6], mac[7], mac[9],
				mac[10], mac[11]);
		FREE(mac);
		return upnp_id;
	}
	return NULL;
}

#ifdef __FreeBSD__
struct sockaddr *sockaddr_dup(struct sockaddr *sa) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct sockaddr *ret;
	socklen_t socklen;
#ifdef HAVE_SOCKADDR_SA_LEN
	socklen = sa->sa_len;
#else
	socklen = sizeof(struct sockaddr_storage);
#endif
	if(!(ret = CALLOC(1, socklen))) {
		logprintf(LOG_ERR, "out of memory");
		exit(EXIT_FAILURE);
	}
	if (ret == NULL)
		return NULL;
	memcpy(ret, sa, socklen);
	return ret;
}

int rep_getifaddrs(struct ifaddrs **ifap) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct ifconf ifc;
	char buff[8192];
	int fd, i, n;
	struct ifreq ifr, *ifrp=NULL;
	struct ifaddrs *curif = NULL, *ifa = NULL;
	struct ifaddrs *lastif = NULL;

	*ifap = NULL;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}

	ifc.ifc_len = sizeof(buff);
	ifc.ifc_buf = buff;

	if (ioctl(fd, SIOCGIFCONF, &ifc) != 0) {
		close(fd);
		return -1;
	}

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifdef __FreeBSD__
#define ifreq_size(i) max(sizeof(struct ifreq),\
     sizeof((i).ifr_name)+(i).ifr_addr.sa_len)
#else
#define ifreq_size(i) sizeof(struct ifreq)
#endif

	n = ifc.ifc_len;

	for (i = 0; i < n; i+= (int)ifreq_size(*ifrp) ) {
		int match = 0;
		ifrp = (struct ifreq *)((char *) ifc.ifc_buf+i);
		for(ifa = *ifap; ifa != NULL; ifa = ifa->ifa_next) {
			if(strcmp(ifrp->ifr_name, ifa->ifa_name) == 0) {
				match = 1;
				break;
			}
		}
		if(match == 1) {
			continue;
		}
		curif = CALLOC(1, sizeof(struct ifaddrs));
		if(curif == NULL) {
			freeifaddrs(*ifap);
			close(fd);
			return -1;
		}

		curif->ifa_name = MALLOC(sizeof(IFNAMSIZ)+1);
		if(curif->ifa_name == NULL) {
			FREE(curif);
			freeifaddrs(*ifap);
			close(fd);
			return -1;
		}
		strncpy(curif->ifa_name, ifrp->ifr_name, IFNAMSIZ);
		strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);

		curif->ifa_flags = (unsigned int)ifr.ifr_flags;
		curif->ifa_dstaddr = NULL;
		curif->ifa_data = NULL;
		curif->ifa_next = NULL;

		curif->ifa_addr = NULL;
		if (ioctl(fd, SIOCGIFADDR, &ifr) != -1) {
			curif->ifa_addr = sockaddr_dup(&ifr.ifr_addr);
			if (curif->ifa_addr == NULL) {
				FREE(curif->ifa_name);
				FREE(curif);
				freeifaddrs(*ifap);
				close(fd);
				return -1;
			}
		}

		curif->ifa_netmask = NULL;
		if (ioctl(fd, SIOCGIFNETMASK, &ifr) != -1) {
			curif->ifa_netmask = sockaddr_dup(&ifr.ifr_addr);
			if (curif->ifa_netmask == NULL) {
				if (curif->ifa_addr != NULL) {
					FREE(curif->ifa_addr);
				}
				FREE(curif->ifa_name);
				FREE(curif);
				freeifaddrs(*ifap);
				close(fd);
				return -1;
			}
		}

		if (lastif == NULL) {
			*ifap = curif;
		} else {
			lastif->ifa_next = curif;
		}
		lastif = curif;
	}

	close(fd);

	return 0;
}

void rep_freeifaddrs(struct ifaddrs *ifaddr) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct ifaddrs *ifa;
	while(ifaddr) {
		ifa = ifaddr;
		FREE(ifa->ifa_name);
		FREE(ifa->ifa_addr);
		FREE(ifa->ifa_netmask);
		ifaddr = ifaddr->ifa_next;
		FREE(ifa);
	}
	FREE(ifaddr);
}
#endif

int whitelist_check(char *ip) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *whitelist = NULL;
	unsigned int client[4] = {0};
	int x = 0, i = 0, error = 1;
	unsigned int n = 0;
	char **array = NULL;
	char wip[16] = {'\0'};

	/* Check if there are any whitelisted ip address */
	if(settings_find_string("whitelist", &whitelist) != 0) {
		return 0;
	}

	if(strlen(whitelist) == 0) {
		return 0;
	}

	/* Explode ip address to a 4 elements int array */
	n = explode(ip, ".", &array);
	x = 0;
	for(x=0;x<n;x++) {
		client[x] = (unsigned int)atoi(array[x]);
		FREE(array[x]);
	}
	if(n > 0) {
		FREE(array);
	}


	if(whitelist_cache == NULL) {
		char *tmp = whitelist;
		x = 0;
		/* Loop through all whitelised ip addresses */
		while(*tmp != '\0') {
			/* Remove any comma's and spaces */
			while(*tmp == ',' || *tmp == ' ') {
				tmp++;
			}
			/* Save ip address in temporary char array */
			wip[x] = *tmp;
			x++;
			tmp++;

			/* Each ip address is either terminated by a comma or EOL delimiter */
			if(*tmp == '\0' || *tmp == ',') {
				x = 0;
				if((whitelist_cache = REALLOC(whitelist_cache, (sizeof(unsigned int ***)*(whitelist_number+1)))) == NULL) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				if((whitelist_cache[whitelist_number] = MALLOC(sizeof(unsigned int **)*2)) == NULL) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				/* Lower boundary */
				if((whitelist_cache[whitelist_number][0] = MALLOC(sizeof(unsigned int *)*4)) == NULL) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				/* Upper boundary */
				if((whitelist_cache[whitelist_number][1] = MALLOC(sizeof(unsigned int *)*4)) == NULL) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}

				/* Turn the whitelist ip address into a upper and lower boundary.
				   If the ip address doesn't contain a wildcard, then the upper
				   and lower boundary are the same. If the ip address does contain
				   a wildcard, then this lower boundary number will be 0 and the
				   upper boundary number 255. */
				i = 0;
				n = explode(wip, ".", &array);
				for(i=0;i<n;i++) {
					if(strcmp(array[i], "*") == 0) {
						whitelist_cache[whitelist_number][0][i] = 0;
						whitelist_cache[whitelist_number][1][i] = 255;
					} else {
						whitelist_cache[whitelist_number][0][i] = (unsigned int)atoi(array[i]);
						whitelist_cache[whitelist_number][1][i] = (unsigned int)atoi(array[i]);
					}
					FREE(array[i]);
				}
				if(n > 0) {
					FREE(array);
				}
				memset(wip, '\0', 16);
				whitelist_number++;
			}
		}
	}

	for(x=0;x<whitelist_number;x++) {
		/* Turn the different ip addresses into one single number and compare those
		   against each other to see if the ip address is inside the lower and upper
		   whitelisted boundary */
		unsigned int wlower = whitelist_cache[x][0][0] << 24 | whitelist_cache[x][0][1] << 16 | whitelist_cache[x][0][2] << 8 | whitelist_cache[x][0][3];
		unsigned int wupper = whitelist_cache[x][1][0] << 24 | whitelist_cache[x][1][1] << 16 | whitelist_cache[x][1][2] << 8 | whitelist_cache[x][1][3];
		unsigned int nip = client[0] << 24 | client[1] << 16 | client[2] << 8 | client[3];

		/* Always allow 127.0.0.1 connections */
		if((nip >= wlower && nip <= wupper) || (nip == 2130706433)) {
			error = 0;
		}
	}

	return error;
}

void whitelist_free(void) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int i = 0;
	if(whitelist_cache) {
		for(i=0;i<whitelist_number;i++) {
			FREE(whitelist_cache[i][0]);
			FREE(whitelist_cache[i][1]);
			FREE(whitelist_cache[i]);
		}
		FREE(whitelist_cache);
	}
}

/* Check if a given file exists */
int file_exists(char *filename) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct stat sb;
	return stat(filename, &sb);
}

/* Check if a given path exists */
int path_exists(char *fil) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	struct stat s;
	char tmp[strlen(fil)+1];
	strcpy(tmp, fil);

	atomiclock();
	/* basename isn't thread safe */
	char *filename = basename(tmp);
	atomicunlock();

	char path[(strlen(tmp)-strlen(filename))+1];
	size_t i = (strlen(tmp)-strlen(filename));

	memset(path, '\0', sizeof(path));
	memcpy(path, tmp, i);
	snprintf(path, i, "%s", tmp);

/*
 * dir stat doens't work on windows if path has a trailing slash
 */
#ifdef _WIN32
	if(path[i-1] == '\\' || path[i-1] == '/') {
		path[i-1] = '\0';
	}
#endif

	if(strcmp(filename, tmp) != 0) {
		int err = stat(path, &s);
		if(err == -1) {
			if(ENOENT == errno) {
				return EXIT_FAILURE;
			} else {
				return EXIT_FAILURE;
			}
		} else {
			if(S_ISDIR(s.st_mode)) {
				return EXIT_SUCCESS;
			} else {
				return EXIT_FAILURE;
			}
		}
	}
	return EXIT_SUCCESS;
}

/* Copyright (C) 1995 Ian Jackson <iwj10@cus.cam.ac.uk> */
/* Copyright (C) 1995 Ian Jackson <iwj10@cus.cam.ac.uk> */
//  1: val > ref
// -1: val < ref
//  0: val == ref
int vercmp(char *val, char *ref) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	int vc, rc;
	long vl, rl;
	char *vp, *rp;
	char *vsep, *rsep;

	if(!val) {
		strcpy(val, "");
	}
	if(!ref) {
		strcpy(ref, "");
	}
	while(1) {
		vp = val;
		while(*vp && !isdigit(*vp)) {
			vp++;
		}
		rp = ref;
		while(*rp && !isdigit(*rp)) {
			rp++;
		}
		while(1) {
			vc =(val == vp) ? 0 : *val++;
			rc =(ref == rp) ? 0 : *ref++;
			if(!rc && !vc) {
				break;
			}
			if(vc && !isalpha(vc)) {
				vc += 256;
			}
			if(rc && !isalpha(rc)) {
				rc += 256;
			}
			if(vc != rc) {
				return vc - rc;
			}
		}
		val = vp;
		ref = rp;
		vl = 0;
		if(isdigit(*vp)) {
			vl = strtol(val, (char**)&val, 10);
		}
		rl = 0;
		if(isdigit(*rp)) {
			rl = strtol(ref, (char**)&ref, 10);
		}
		if(vl != rl) {
			return (int)(vl - rl);
		}

		vc = *val;
		rc = *ref;
		vsep = strchr(".-", vc);
		rsep = strchr(".-", rc);

		if((vsep && !rsep) || !*val) {
			return 0;
		}

		if((!vsep && rsep) || !*ref) {
			return +1;
		}

		if(!*val && !*ref) {
			return 0;
		}
	}
}

char *uniq_space(char *str){
	char *from = NULL, *to = NULL;
	int spc=0;
	to = from = str;
	while(1){
		if(spc == 1 && *from == ' ' && to[-1] == ' ') {
			++from;
		} else {
			spc = (*from == ' ') ? 1 : 0;
			*to++ = *from++;
			if(!to[-1]) {
				break;
			}
		}
	}
	return str;
}

int str_replace(char *search, char *replace, char **str) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	char *target = *str;
	unsigned short match = 0;
	int len = (int)strlen(target);
	int nlen = 0;
	int slen = (int)strlen(search);
	int rlen = (int)strlen(replace);
	int x = 0;

	while(x < len) {
		if(strncmp(&target[x], search, (size_t)slen) == 0) {
			match = 1;
			int rpos = (x + (slen - rlen));
			if(rpos < 0) {
				slen -= rpos;
				rpos = 0;
			}
			nlen = len - (slen - rlen);
			if(len < nlen) {
				if((target = REALLOC(target, (size_t)nlen+1)) == NULL) {
					logprintf(LOG_ERR, "out of memory");
					exit(EXIT_FAILURE);
				}
				memset(&target[len], '\0', (size_t)(nlen-len));
			}
			len = nlen;

			memmove(&target[x], &target[rpos], (size_t)(len-x));
			strncpy(&target[x], replace, (size_t)rlen);
			target[len] = '\0';
			x += rlen-1;
		}
		x++;
	}
	if(match == 1) {
		return (int)len;
	} else {
		return -1;
	}
}

int stricmp(char const *a, char const *b) {
	logprintf(LOG_STACK, "%s(...)", __FUNCTION__);

	for(;; a++, b++) {
			int d = tolower(*a) - tolower(*b);
			if(d != 0 || !*a)
				return d;
	}
}