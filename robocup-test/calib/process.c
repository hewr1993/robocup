#include <fcntl.h>
#include <sys/mman.h>
#include "net.h"
#include "cmv.h"

int use_cmv1 = 0;
int use_cmv2 = 0;
int DataInputMode = 0;

struct cmv_data data1;
struct cmv_data data2;
struct net ns1, ns2;

int use_cmv = 0;
struct cmv_data data;
unsigned char src[YUV_SIZE];
char *filename;

unsigned char *process(int argc, char *argv[]) {

	int port1 = 8000, port2 = 8001;
	char cfilename1[64];
        char cfilename2[64];

	for (;;) {
		int t = getopt(argc, argv, "l:r:bp:h:s");
		if (t == -1) {
			break;
		}

		switch (t) {
			case 'l':
				if (sscanf(optarg, "%s", cfilename1) != 1) {
					fprintf(stderr, "error optarg: %s\n", optarg);
					exit(-1);
				}
				use_cmv1 = 1;
				break;
			case 'r':
				if (sscanf(optarg, "%s", cfilename2) != 1) {
					fprintf(stderr, "error optarg: %s\n", optarg);
					exit(-1);
				}
				use_cmv2 = 1;
				break;
			case 'p':
				if (sscanf(optarg, "%d,%d", &port1, &port2) != 2) {
					fprintf(stderr, "error optarg: %s\n", optarg);
					exit(-1);
				}
				break;
			case 'h':
				fprintf(stderr, "Usage: %s [option]\n"
						"-c s\tset color file s\n"
						"-p n,m\tset port n (default 8000,8001)\n"
						"-h\tprint this help\n", argv[0]);
				exit(0);
			case '?':
				break;
			case 's':
				printf("-s detected\n");
				DataInputMode = 1;
				break;
			default:
				fprintf(stderr, "option error: %d\n", t);
				exit(-1);
		}
	}

	if (argc - optind != 1) {
		exit(-1);
	}

  filename = argv[optind];
  
  /*
	nets_init(&ns1, port1);
	nets_init(&ns2, port2);
	printf("using port %d %d\n", port1, port2);
  */

	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror(filename);
		exit(-1);
	}
	
	int filelength=lseek(fd,0,SEEK_END);

	char *suffix = strrchr(filename, '.');
	if (suffix == NULL) {
		fprintf(stderr, "unknown filename: %s\n", filename);
		exit(-1);
	}
	++suffix;

  if (!strcmp(suffix, "yuv")) {
    unsigned ori= mmap(NULL, YUV_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (ori == MAP_FAILED) {
      perror("mmap");
      exit(-1);
    }
    memcpy(src,ori,YUV_SIZE);
    //dec_init(W,H,R);
    //dec_process(raw, src,W,H,R);
  }
	else {
		fprintf(stderr, "unknown filename: %s\n", filename);
		exit(-1);
	}
	if (use_cmv1) {
		cmv_init(&data1, cfilename1);
		printf("using color file %s\n", cfilename1);
	}
  if (use_cmv2) {
		cmv_init(&data2, cfilename2);
		printf("using color file %s\n", cfilename2);
	}
  return src;
}
