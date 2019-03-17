#include <iostream>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
using namespace std;

typedef struct{
	int frame_no;
	int valid;
}page_table_node;

typedef struct{
	int pid;
	int num_pages;
}process_page_map_node;

void sig_handler(int signo)
{
	if(signo == SIGUSR1){
		// do nothing
	}
}
int main()
{
	int k=3,m=4,f=20;
	signal(SIGUSR1,sig_handler);
	// cin>>k>>m>>f;
	// Init data structures
	
	// vector<int> free_frame_list(f,1);  	// Each index is a frame, 0 - occupied, 1 - free
	// a k x m matrix, each entry is a tuple (frame number, valid bit)
	// vector<vector<pair<int,bool> > > page_tables = init_page_tables(k,m);
	srand(time(NULL));
	key_t mq1,mq2,mq3,sm1,sm2,sm3;
	int mq1_id,mq2_id,mq3_id,sm1_id,sm2_id,sm3_id;
	mq1 = ftok("mq1",65);		// message queue 1, from master to scheduler
	mq2 = ftok("mq2",65);		// message queue 2, from mmu to scheduler
	mq3 = ftok("mq3",65);		// message queue 3, from process to mmu
	sm1 = ftok("sm1",65);
	sm2 = ftok("sm2",65);
	sm3 = ftok("sm3",65);
	sm1_id = shmget(sm1,m*k*sizeof(page_table_node),IPC_CREAT|0666);
	sm2_id = shmget(sm2,f*sizeof(int),IPC_CREAT|0666);
	sm3_id = shmget(sm3,k*sizeof(process_page_map_node),IPC_CREAT|0666);
	mq1_id = msgget(mq1,IPC_CREAT|0666);
	mq2_id = msgget(mq2,IPC_CREAT|0666);
	mq3_id = msgget(mq3,IPC_CREAT|0666);

	page_table_node* page_tables = (page_table_node*)shmat(sm1_id,NULL,0);
	int* free_frame_list = (int *)shmat(sm2_id,NULL,0);
	process_page_map_node* process_page_map = (process_page_map_node*)shmat(sm3_id,NULL,0);

	for(int i=0;i<k;i++){
		for(int j=0;j<m;j++){
			page_tables[i*m+j].frame_no = -1;
			page_tables[i*m+j].valid = 0;
		}
	}

	for(int i=0;i<f;i++){
		free_frame_list[i] = 1;
	}

	pid_t pid_scheduler = fork();
	if(pid_scheduler==0){
		// exec call to scheduler.cpp
		char** args = new char*[5];
		args[0] = "./scheduler";
		string mq1_id_to_str = to_string(mq1_id);
		args[1] = new char[mq1_id_to_str.size()+1];
		strcpy(args[1],mq1_id_to_str);
		string mq2_id_to_str = to_string(mq2_id);
		args[2] = new char[mq2_id_to_str.size()+1];
		strcpy(args[2],mq2_id_to_str);
		string k_str = to_string(k);
		args[3] = new char[k_str.size()+1];
		strcpy(args[3],k_str);
		args[4] = NULL;
		execvp(args[0],args);
	}
	pid_t pid_mmu = fork();
	if(pid_mmu == 0){
		// exec call to mmu
		char** args = new char* [9];
		args[0] = "./mmu";
		string mq2_id_to_str = to_string(mq2_id);
		args[1] = new char[mq2_id_to_str.size()+1];
		strcpy(args[1],mq2_id_to_str);
		string mq3_id_to_str = to_string(mq3_id);
		args[2] = new char[mq3_id_to_str.size()+1];
		strcpy(args[2],mq3_id_to_str);
		string sm1_id_to_str = to_string(sm1_id);
		args[3] = new char[sm1_id_to_str.size()+1];
		strcpy(args[3],sm1_id_to_str);
		string sm2_id_to_str = to_string(sm2_id);
		args[4] = new char[sm2_id_to_str.size()+1];
		strcpy(args[4],sm2_id_to_str);
		string sm3_id_to_str = to_string(sm3_id);
		args[5] = new char[sm3_id_to_str.size()+1];
		strcpy(args[5],sm3_id_to_str);
		string k_str = to_string(k);
		args[6] = new char[k_str.size()+1];
		strcpy(args[6],k_str);
		string m_str = to_string(m);
		args[7] = new char[m_str.size()+1];
		strcpy(args[7],m_str);
		args[8] = NULL;
		execvp(args[0],args);
	}
	pid_t processes[k];
	for(int i=0;i<k;i++){
		// create pr string
		int mi = rand()%m+1;
		int page_ref_len = rand()%(8*mi+1) + (2*mi);
		string pr_str = "";
		for(int j=0;j<page_ref_len;j++){
			pr_str += to_string(rand()%mi);
			pr_str += "|";
		}
		processes[i] = fork();
		if(processes[i] == 0){
			// exec, parameters - pr string, mq1, mq3
			char** args = new char* [5];
			// possible place for segfault;
			args[0] = "./proc";
			args[1] = new char[pr_str.size()+1];
			strcpy(args[1],pr_str);
			string mq1_id_to_str = to_string(mq1_id);
			args[2] = new char[mq1_id_to_str.size()+1];
			strcpy(args[2],mq1_id_to_str);
			string mq3_id_to_str = to_string(mq3_id);
			args[3] = new char[mq3_id_to_str.size()+1];
			strcpy(args[3],mq3_id_to_str);
			args[4] = NULL;
			execvp(args[0],args);
			printf("Error\n");
		}
		else{
			process_page_map[i].pid = processes[i];
			process_page_map[i].num_pages = mi;
			int* process_pid = new int;
			*process_pid = processes[i];
			msgsnd(mq1_id,process_pid,sizeof(int),0);
			usleep(250000);
		}

	}
	pause();
	kill(pid_scheduler,SIGKILL);
	kill(pid_mmu,SIGKILL);
	for(int i=0;i<k;i++){
		kill(processes[i],SIGKILL);
	}
	printf("All done!\n");
	return 0;
}