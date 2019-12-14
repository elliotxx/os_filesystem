#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

//�궨��
#define BLOCK_SIZE	512						//��Ŵ�СΪ512B
#define INODE_SIZE	128						//inode�ڵ��СΪ128B��ע��sizeof(Inode)���ܳ�����ֵ
#define MAX_NAME_SIZE 28					//������ֳ��ȣ�����ҪС�������С

#define INODE_NUM	640						//inode�ڵ���,���64���ļ�
#define BLOCK_NUM	10240					//�������10240 * 512B = 5120KB
#define BLOCKS_PER_GROUP	64				//���п��ջ��С��һ�����ж�ջ����ܴ���ٸ����̿��ַ

#define MODE_DIR	01000					//Ŀ¼��ʶ
#define MODE_FILE	00000					//�ļ���ʶ
#define OWNER_R	4<<6						//���û���Ȩ��
#define OWNER_W	2<<6						//���û�дȨ��
#define OWNER_X	1<<6						//���û�ִ��Ȩ��
#define GROUP_R	4<<3						//���û���Ȩ��
#define GROUP_W	2<<3						//���û�дȨ��
#define GROUP_X	1<<3						//���û�ִ��Ȩ��
#define OTHERS_R	4						//�����û���Ȩ��
#define OTHERS_W	2						//�����û�дȨ��
#define OTHERS_X	1						//�����û�ִ��Ȩ��

// ע umask Ĭ��Ϊ002������ļ�Ĭ��Ȩ�� 664��Ŀ¼Ĭ��Ȩ��775
#define FILE_DEF_PERMISSION 0664			//�ļ�Ĭ��Ȩ��
#define DIR_DEF_PERMISSION	0775			//Ŀ¼Ĭ��Ȩ�� 

#define FILESYSNAME	"MingOS.sys"			//��������ļ���


//�ṹ������
//������
struct SuperBlock{
	unsigned short s_INODE_NUM;				//inode�ڵ�������� 65535
	unsigned int s_BLOCK_NUM;				//���̿��������� 4294967294

	unsigned short s_free_INODE_NUM;		//����inode�ڵ���
	unsigned int s_free_BLOCK_NUM;			//���д��̿���
	int s_free_addr;						//���п��ջָ��
	int s_free[BLOCKS_PER_GROUP];			//���п��ջ

	unsigned short s_BLOCK_SIZE;			//���̿��С
	unsigned short s_INODE_SIZE;			//inode��С
	unsigned short s_SUPERBLOCK_SIZE;		//�������С
	unsigned short s_blocks_per_group;		//ÿ blockgroup ��block����

	//���̷ֲ�
	int s_Superblock_StartAddr;
	int s_InodeBitmap_StartAddr;
	int s_BlockBitmap_StartAddr;
	int s_Inode_StartAddr;
	int s_Block_StartAddr;
};

//inode�ڵ�
struct Inode{
	unsigned short i_ino;					//inode��ʶ����ţ�
	unsigned short i_mode;					//��ȡȨ�ޡ�r--��ȡ��w--д��x--ִ��
	unsigned short i_cnt;					//���������ж����ļ���ָ�����inode
	//unsigned short i_uid;					//�ļ������û�id
	//unsigned short i_gid;					//�ļ������û���id
	char i_uname[20];						//�ļ������û�
	char i_gname[20];						//�ļ������û���
	unsigned int i_size;					//�ļ���С����λΪ�ֽڣ�B��
	time_t  i_ctime;						//inode��һ�α䶯��ʱ��
	time_t  i_mtime;						//�ļ�������һ�α䶯��ʱ��
	time_t  i_atime;						//�ļ���һ�δ򿪵�ʱ��
	int i_dirBlock[10];						//10��ֱ�ӿ顣10*512B = 5120B = 5KB
	int i_indirBlock_1;						//һ����ӿ顣512B/4 * 512B = 128 * 512B = 64KB
	//unsigned int i_indirBlock_2;			//������ӿ顣(512B/4)*(512B/4) * 512B = 128*128*512B = 8192KB = 8MB
	//unsigned int i_indirBlock_3;			//������ӿ顣(512B/4)*(512B/4)*(512B/4) * 512B = 128*128*128*512B = 1048576KB = 1024MB = 1G
											//�ļ�ϵͳ̫С������ʡ�Զ�����������ӿ�
};

//Ŀ¼��
struct DirItem{								//32�ֽڣ�һ�����̿��ܴ� 512/32=16 ��Ŀ¼��
	char itemName[MAX_NAME_SIZE];			//Ŀ¼�����ļ���
	int inodeAddr;							//Ŀ¼���Ӧ��inode�ڵ��ַ
};

//ȫ�ֱ�������
extern SuperBlock *superblock;
extern const int Inode_StartAddr;
extern const int Superblock_StartAddr;		//������ ƫ�Ƶ�ַ,ռһ�����̿�
extern const int InodeBitmap_StartAddr;		//inodeλͼ ƫ�Ƶ�ַ��ռ�������̿飬����� 1024 ��inode��״̬
extern const int BlockBitmap_StartAddr;		//blockλͼ ƫ�Ƶ�ַ��ռ��ʮ�����̿飬����� 10240 �����̿飨5120KB����״̬
extern const int Inode_StartAddr;			//inode�ڵ��� ƫ�Ƶ�ַ��ռ INODE_NUM/(BLOCK_SIZE/INODE_SIZE) �����̿�
extern const int Block_StartAddr;			//block������ ƫ�Ƶ�ַ ��ռ INODE_NUM �����̿�
extern const int File_Max_Size;				//�����ļ�����С
extern const int Sum_Size;					//��������ļ���С


//ȫ�ֱ�������
extern int Root_Dir_Addr;					//��Ŀ¼inode��ַ
extern int Cur_Dir_Addr;					//��ǰĿ¼
extern char Cur_Dir_Name[310];				//��ǰĿ¼��
extern char Cur_Host_Name[110];				//��ǰ������
extern char Cur_User_Name[110];				//��ǰ��½�û���
extern char Cur_Group_Name[110];			//��ǰ��½�û�����
extern char Cur_User_Dir_Name[310];			//��ǰ��½�û�Ŀ¼��

extern int nextUID;							//��һ��Ҫ������û���ʶ��
extern int nextGID;							//��һ��Ҫ������û����ʶ��

extern bool isLogin;						//�Ƿ����û���½

extern FILE* fw;							//��������ļ� д�ļ�ָ��
extern FILE* fr;							//��������ļ� ���ļ�ָ��
extern SuperBlock *superblock;				//������ָ��
extern bool inode_bitmap[INODE_NUM];		//inodeλͼ
extern bool block_bitmap[BLOCK_NUM];		//���̿�λͼ

extern char buffer[10000000];				//10M������������������ļ�


//��������
void Ready();													//��¼ϵͳǰ��׼������,ע��+��װ
bool Format();													//��ʽ��һ����������ļ�
bool Install();													//��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
void printSuperBlock();											//��ӡ��������Ϣ
void printInodeBitmap();										//��ӡinodeʹ�����
void printBlockBitmap(int num = superblock->s_BLOCK_NUM);		//��ӡblockʹ�����
int	 balloc();													//���̿���亯��
bool bfree();													//���̿��ͷź���
int  ialloc();													//����i�ڵ�������
bool ifree();													//�ͷ�i���������
bool mkdir(int parinoAddr,char name[]);							//Ŀ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼��
bool rmdir(int parinoAddr,char name[]);							//Ŀ¼ɾ������
bool create(int parinoAddr,char name[],char buf[]);				//�����ļ�����
bool del(int parinoAddr,char name[]);							//ɾ���ļ����� 
void ls(int parinoaddr);										//��ʾ��ǰĿ¼�µ������ļ����ļ���
void cd(int parinoaddr,char name[]);							//���뵱ǰĿ¼�µ�nameĿ¼
void gotoxy(HANDLE hOut, int x, int y);							//�ƶ���굽ָ��λ��
void vi(int parinoaddr,char name[],char buf[]);					//ģ��һ����vi�������ı�
void writefile(Inode fileInode,int fileInodeAddr,char buf[]);	//��buf����д���ļ��Ĵ��̿�
void inUsername(char username[]);								//�����û���
void inPasswd(char passwd[]);									//��������
bool login();													//��½����
bool check(char username[],char passwd[]);						//�˶��û���������
void gotoRoot();												//�ص���Ŀ¼
void logout();													//�û�ע��
bool useradd(char username[]);									//�û�ע��
bool userdel(char username[]);									//�û�ɾ��
void chmod(int parinoAddr,char name[],int pmode);				//�޸��ļ���Ŀ¼Ȩ��
void touch(int parinoAddr,char name[],char buf[]);				//touch������ļ��������ַ�
void help();													//��ʾ���������嵥

void cmd(char str[]);											//�������������
