#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<time.h>
#include<pthread.h>
// Each partion is 30KB ~ 70KB,
// and total memory is 640KB,so the
// maximun item is 22
#define MAX_ITEM 22
#define MAX_QUEUE 22
#define MAX_JOBS 22
#define LIMIT 10


// Record the item number of Free Partition Table
int counter;

/*
 * A item of Free Partition Table
 *
 */
struct Item
{
	unsigned int id;   // partition number
	unsigned int size;  // partition size
	void *start;     // partition begin address
	bool flag;    // True denote available,False denote not available
};

// Create Free Partition Table
struct Item freePartTable[MAX_ITEM];


/*
 * Record the memory requirement
 *
 */
struct Apply
{
	unsigned int apply_size;
	unsigned int resident_time;
};

/*
 * A item of Jobs Table
 *
 */
struct Job
{
	//void *startAddr;
	unsigned int size;
	unsigned int time;
	bool flag;   
	unsigned int partID;
};

/*
 * Save the application in queue
 *
 */
struct Apply applyQueue[MAX_QUEUE];
int head = 0;
int tail = 0;
int queueLen = 0;

// Save the jobs
struct Job jobs[MAX_JOBS];

/*
 * Initial jobs
 *
 */
void InitJobs(struct Job jobs[])
{
	for(int i = 0; i < MAX_JOBS; ++i)
		jobs[i].flag = false;
}


/*
 * Initial Free Partition Table
 *
 */
int InitFPT(struct Item table[],void *begin)
{
	srand(time(NULL));
	int tmp = rand() % 30 + 40;
	int sum = tmp;

	// Initial `table[0]`
	table[0].id = 1;
	table[0].size = tmp;
	table[0].start = begin;
	table[0].flag = true;

	// Item's counter
	int counter = 1;

	for(int i = 1; i < MAX_ITEM; ++i)
	{
		tmp = rand() % 30 + 40;
		sum += tmp;
		
		table[i].id = i+1;
		table[i].start = table[i-1].start + table[i-1].size * 1024;
		table[i].flag = true;

		if(sum <= 640)
		{
			table[i].size = tmp;
			++counter;
		}
		else
		{
			table[i].size = 640 - (sum - tmp);
			++counter;
			break;
		}

	}

	// Return the number of item that actually used
	return counter;
}

/*
 * Product jobs
 *
 */
void *product(void *arg)
{
	int sleepTime = 0;
	while(1)
	{
		if(queueLen < MAX_QUEUE)
		{
			srand(time(NULL));
			applyQueue[tail].apply_size = 30 + rand() % 41;
			applyQueue[tail].resident_time = 2 + rand() % 9;
			tail = (tail+1) % MAX_QUEUE;
			++queueLen;
		}
		else
			printf("Memory is insufficient.\n");
		
		// `sleepTime` in range [2,3]
		sleepTime = 2 + rand() % 2;
		sleep(sleepTime);
	}
}

/*
 * Memory allocation
 *
 */
void *allocateMemory(void *arg)
{
	// `size`: application `size` KB memeory
	// `time`: resident time
	int size,time;

	while(1)
	{
		if(queueLen > 0)
		{
			// Get the information of application
			size = applyQueue[head].apply_size;
			time = applyQueue[head].resident_time;

			// Modify the `applyQueue`
			head = (head+1) % MAX_QUEUE;
			--queueLen;

			// Find the first available item in Jobs Table
			int j;
			for(j = 0; j < MAX_JOBS; ++j)
			{
				// Available
				if(!jobs[j].flag)
					break;
			}

			if(j == MAX_JOBS)
				fprintf(stderr,"Some error happend in allocateMemory function.\n");

			// First Fit Algorithm
			for(int i = 0; i < MAX_ITEM; ++i)
			{
				// First fit
				if(freePartTable[i].flag && freePartTable[i].size >= size)
				{
					//jobs[j].startAddr = freePartTable[i].start;

					// jobs[j].time in range [2,10]
					jobs[j].time = 5 + rand() % 9;
					jobs[j].flag = true;
					jobs[j].partID = freePartTable[i].id;

					// Allocate the entire partition
					if(freePartTable[i].size - size <= LIMIT)
					{
						jobs[j].size = freePartTable[i].size;

						// Update Free Partition Table
						freePartTable[i].start += freePartTable[i].size * 1024;
						freePartTable[i].size = 0;

						// Mark this partition is not available
						freePartTable[i].flag = false;

					}
					// Allocate part of partition
					else
					{
						jobs[j].size = size;

						// Update Free Partiton Table
						freePartTable[i].size -= size;
						freePartTable[i].start += size * 1024;
					}

					break;
				}
			}

		}
		else
			sleep(2);
	}
}

int reclaimJobs[MAX_JOBS] = {0,};
int reclaimJobs_len = 0;

/*
 * Reclaim memory
 *
 */
void *reclaim(void *arg)
{
	sleep(1);
	while(1)
	{
		for(int i = 0; i < MAX_JOBS; ++i)
		{
			if(jobs[i].flag)
			{
				--(jobs[i].time);

				// Should be reclaimed
				if(jobs[i].time <= 0)
				{
					reclaimJobs[reclaimJobs_len++] = i;
					jobs[i].flag = false;
				}
			}
		}

		// Some jobs should be reclaimed
		if(reclaimJobs_len)
		{
			int index = 0, partID = 0;
			while(reclaimJobs_len)
			{
				index = reclaimJobs[--reclaimJobs_len];
				partID = jobs[index].partID;

				// Update freePartTable[]
				freePartTable[partID-1].size += jobs[index].size;
				freePartTable[partID-1].start -= jobs[index].size * 1024;
				if(!freePartTable[partID-1].flag)
					freePartTable[partID-1].flag = true;
			}
		}

		sleep(1);
	}
}



/*
 * Show information of Free Partition Table
 * and Jobs Table
 */
void *showInfo(void *arg)
{
	while(1)
	{
		system("clear");

		printf("##################################### Free Partition Table ##########################################\n\n");
		printf("%15s %15s %15s %15s\n\n","ID","SIZE(KB)","ADDRESS","STATUS");
		for(int i = 0; i < counter; ++i)
		{
			printf("%15d %15d %15p %15s\n",i+1,freePartTable[i].size,freePartTable[i].start,\
					freePartTable[i].flag ? "Available" : "Blank");
		}
		printf("\n#####################################################################################################\n\n");

		
		char tmp[10] = {0,};
		printf("************************************** Resident Memory Jobs *****************************************\n\n");
		printf("%20s %25s %25s\n\n","JOBS","RESIDENT TIME(sec)","SIZE(KB)");
		for(int i = 0; i < MAX_JOBS; ++i)
		{
			if(jobs[i].flag)
			{
				sprintf(tmp,"Job%d",i+1);
				printf("%20s %25d %25d\n",tmp,jobs[i].time,jobs[i].size);
			}
		}
		printf("\n*****************************************************************************************************\n\n");

		sleep(2);
	}
}

/*
 * Driver Program
 *
 */
int main(void)
{
	pthread_t tid1,tid2,tid3,tid4;
	void *begin = malloc(1024*640);

	// Initial Free Partition Table
	counter = InitFPT(freePartTable,begin);

	// Initial Jobs Table
	InitJobs(jobs);

	// Product Jobs
	pthread_create(&tid1,NULL,product,NULL);

	// Allocate Memory For Jobs
	pthread_create(&tid2,NULL,allocateMemory,NULL);

	// Reclaim the Memory
	pthread_create(&tid3,NULL,reclaim,NULL);

	// Show Information Abount Free Partition Table
	// And Jobs In Memory
	pthread_create(&tid4,NULL,showInfo,NULL);
	
	getchar();

	system("clear");

	free(begin);
	return 0;
}
