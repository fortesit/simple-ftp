#ifndef __MYFTP__

#define __MYFTP__

struct message_s {
	char protocol[6];	/* protocol magic number (6 bytes) */
	char type;	/* type (1 byte) */
	char status;	/* status (1 byte) */
	int length;	/* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

#endif
