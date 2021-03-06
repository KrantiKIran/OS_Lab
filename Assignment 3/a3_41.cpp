#include <iostream>
#include <bits/stdc++.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>

// Mean for exponential distribution
#define LAMBDA 0.4
// Time quantum for Round Robin
#define DELTA 2
using namespace std;

// Function forward declarations
vector<int> generate_uniform(int n);
vector<int> generate_exponential(int n);
double fcfs_awt(vector<pair<int,int> > &processes);
double non_preemptive_sjf_atn(vector<pair<int,int> > p);
double round_robin(vector<pair<int,int> > &processes);
double preemptive_sjf_atn(vector<pair<int,int> > p);
double hrn_att(vector<pair<int,int> > p);
double pre_hrn_att(vector<pair<int,int> > p);

int main()
{
	int n;
	cin>>n;// Obtain the number of processes to be scheduled
	vector<int> burst_times = generate_uniform(n);// get the burst times
	vector<int> arrival_times = generate_exponential(n);// get the corresponding arrival times
	ofstream fout,fout_lower_bound;// Files to write the outputs to 
	// fout contains the values of the burst and arrival times for one session. 
	// fout_lower_bound contains the average burst times and the average turnaround times for all the processes
	
	fout.open("cpu_times.txt");
	fout_lower_bound.open("fcfs_lower_bound.txt",ofstream::app);
	for(int i=0;i<n;i++){
		fout<<arrival_times[i]<<"\t"<<burst_times[i]<<endl;
	}

	fout_lower_bound<<"N = "<<n<<": ";
	
	//find the average burst times to get the theoretical lower bound of FCFS
	double avg_burst_time = (double)accumulate(burst_times.begin(),burst_times.end(),0)/(double)(burst_times.size());
	fout_lower_bound<<avg_burst_time<<"\t";
	fout.close();
	// Consider each process as a tuple of (arrival time, burst time).
	// store the arrival and burst times as a vector of pairs
	vector<pair<int,int> > processes;
	for(int i=0;i<n;i++){
		processes.push_back(pair<int,int>(arrival_times[i],burst_times[i]));
	}

	// Obtain the average turnaround times and print them
	double fcfs_avg_awt = fcfs_awt(processes);
	cout<<fcfs_avg_awt<<endl;// FCFS
	fout_lower_bound<<fcfs_avg_awt<<endl;
	double sjf_avg = non_preemptive_sjf_atn(processes);
	cout<<sjf_avg<<endl;//Non Preemptive SJF
	double round_robin_avg_awt = round_robin(processes);
	cout<<round_robin_avg_awt<<endl;//Round Robin
	double psjf_avg = preemptive_sjf_atn(processes);
	cout<<psjf_avg<<endl;// Preemptive SJF
	double hrn_avg = hrn_att(processes);
	cout<<hrn_avg<<endl;// Non Preemptive HRRN
	double pre_hrn_avg = pre_hrn_att(processes);
	cout<<pre_hrn_avg<<endl; // Preemptive HRRN
  	return 0;
}

/*
Function to generate the burst times
Returns a vector of n randomly generated numbers between 1 and 20
*/
vector<int> generate_uniform(int n)
{
	vector<int> v;
	srand(time(NULL));
	for(int i=0;i<n;i++){
		v.push_back(rand()%20+1);
	}
	return v;
}

// Function to generate n-1 exponential random numbers - for inter arrival times of the processes
// and compute the actual arrival times using these inter arrival times.
/*
	For generating the exponential distribution, we use the property that:-

	If R is a uniform random number in the range (0,1) a random variable from an 
	exponential distribution with mean LAMBDA can be generated as 
		(-1.0/LAMBDA)*ln(R)
*/
vector<int> generate_exponential(int n)
{
	vector<int> arrival_times;
	arrival_times.push_back(0); // First process arrives at time 0
	srand(time(NULL));
	int i=1; 	// Loop counter 
	while(i<n){
		// Generate uniform random number in range (0,1) by dividing the 
		// generated random number by RAND_MAX + 1.
		double uni_rand = (float)rand() / (RAND_MAX + 1.0);
		// Ignore if the number is zero, since log is not defined
		if(uni_rand != 0){
			double val = -log(uni_rand) / LAMBDA;
			// We need exponential numbers in range [0,10]. so ignore higher values
			if(val <=10){
				/*
				Compute the new process's arrival time by adding the 
				generated exponential random variable value to the 
				arrival time of the previous process
				*/
				int new_val = arrival_times[arrival_times.size()-1]+val;
				arrival_times.push_back(new_val);
				i++;
			}
		}
	}
	return arrival_times;
}


/* 
Function to find ATT in the First come first serve scheduler
We calculate the wait time at each step and add it to the
burst time of a process to obtain the average turnaround time
*/
double fcfs_awt(vector<pair<int,int> > &processes)
{
	int n = processes.size();
	vector<int> wait_times;			// Array to store the wait times
	// First process has no wait time, hence initialise total turnaround time to burst time of first process
	int total_turnaround_time = processes[0].second;	
	wait_times.push_back(0);
	for(int i=1;i<n;i++){
		/*
			For a given process i, waittime is equal to the time taken for all the previous processes to complete,
			wt[i] = (bt[0] + bt[1] +...... bt[i-1]) - at[i]
			i.e, wt[i+1] = wt[i] + at[i] + bt[i] - at[i+1]
			or, wt[i] = wt[i-1] + at[i-1] + bt[i-1] -at[i]
			Turn around time of a process = wait time + burst time.
		*/
		int wait_time = wait_times[i-1] + processes[i-1].first + processes[i-1].second - processes[i].first;
		wait_times.push_back(wait_time);
		// Add the turnaround time of this process = wait time + burst time
		total_turnaround_time += (wait_time + processes[i].second);	
	}
	return (double)total_turnaround_time/(double)n;	// Compute the average turn around time and return the value

}

//The function used as a comparision for the priority queue.\
Using this the priority queue can tell which has the shortest time.
bool compare(pair<int,int> a,pair<int,int> b)
{
	return a.second>b.second;
}

/*
	Function to compute the average turnaround time for Non Preemptive Shortest Job First Scheduling
	Here, we make use of a min heap sorted by burst times to get the shortest job first evertime.
*/
double non_preemptive_sjf_atn(vector<pair<int,int> > p)
{
	int turnaround_time = 0;// the variable to store the total turnaround time

	int current_time = 0;// the variable to keep track of the current time

	// the heap to find the shortest processes among all the process that have appeared
	priority_queue<pair<int,int>, vector<pair<int,int> >,bool(*)(pair<int,int>,pair<int,int>)> heap(compare);

	/*
	For any given process, the turnaround time is defined as the total
	time from the arrival till the end of completion
	The variable current_time is essentially the time elapsed till then at any stage
	and is equal to completion time of the ith process at that stage.
	we subtract the completion time of the ith process
	from the arrival time to finally end up with the turnaround time for that 
	particular process, and then add this to the total turnaround time
	*/
	int n = p.size();
	int i=-1;// to keep number of process done
	int j=0;// to keep track of the number of processes that have been added to the heap

	while(++i<n)// while there are still processes remaining
	{
		// add all the processes that have already arrived into the queue
		for(;j<n && p[j].first<=current_time;j++)
		{
			heap.push(p[j]);
		}

		// if the heap is empty, i.e the processe arrive later but all that have arrived are over \
		then set the current time to the arrival time of the next closest process.
		if(heap.empty() && j<n)
		{
			current_time = p[j].first;
		}

		pair<int,int> curr = heap.top();// find the shortest process, 
		heap.pop();// remove it from the queue
		// Add the execution time to the total elapsed time
		current_time += curr.second;
		turnaround_time += current_time - curr.first; // add its turanaround to the total
	}

	//Then we return the average
	return (double)turnaround_time/(p.size());
}

/*
	Function to compute the average turnaround time for Round Robin Scheduling with time quantum delta = 2
*/
double round_robin(vector<pair<int,int> > &processes)
{
	int delta = DELTA;			// the time quantum
	// Initialise a simple FIFO queue to hold the jobs that are in ready state.
	// Each process now is considered as a ((arrival time, burst time), remaining time) tuple.
	queue<pair<pair<int,int>, int> > jobs;
	int n = processes.size();// number of processes
	int current_time = 0;// variable to find the current time
	int i=n;		// Loop counter, indicating number of processes yet to be scheduled.
	int j=0;
	int total_turnaround_time = 0; // find the total turnaround time

	while(i){

		// Add all the processes that have arrived till the current time into the queue
		for(;j<n && processes[j].first <= current_time;j++){
			jobs.push(pair<pair<int,int>, int> (processes[j],processes[j].second));
		}
		// if the queue is not empty
		if(!jobs.empty()){
			pair<pair<int,int>, int> curr = jobs.front();	// get the first process in the queue
			jobs.pop();										// and remove it from the queue
			// If there are any processes in the queue, 
			if(curr.second > delta){	
				// Run this for delta time, decrement time remaining by time quantum
				curr.second -= delta;
				current_time += delta;// increment current time by delta

				// Add all the processes that have arrived till the current time into the queue
				for(;j<n && processes[j].first <= current_time;j++){
					jobs.push(pair<pair<int,int>, int> (processes[j],processes[j].second));
				}
				jobs.push(curr);// push the job as there is still some time left in it
			}
			else if(curr.second > 0){// if less than a time quantum time is remaining
				current_time += curr.second;// increment the current time by the amount of time remaining
				total_turnaround_time += (current_time - curr.first.first);// increment the total turnaround time
				curr.second = 0;// set the time remaining to zero 
				// and decrement the number of processes remaining
				i--;
			}
		}
		else current_time++;	// else just increment the time and wait for processes to arrive
	}
	// return the average of the turnaround time
	return (double)total_turnaround_time/(double)n;
}


// The overloaded compare function used in the min-priority queue for the preemptive sjf 
bool compare(pair<pair<int,int>,int> a,pair<pair<int,int>,int> b)
{
	return a.second>b.second;
}

/*The preemptive version of the SJF*/
double preemptive_sjf_atn(vector<pair<int,int> > p)
{
	int turnaround_time = 0;// The total turnaround time

	int current_time = 0;	// Current time

	// Heap to help in getting the shortest process
	// Here we use a (pair<int,int>,int) pair where the third int stores the time remaining in the process
	priority_queue<pair<pair<int,int>,int>,
					vector<pair<pair<int,int>,int> >,
					bool(*)(pair<pair<int,int>,int>,pair<pair<int,int>,int>)> heap(compare);


	int n = p.size();
	int i=0;
	int j=0;
	// While there are processes left
	while(i<n)
	{
		// Puch all processes that have arrived	
		for(;j<n && p[j].first<=current_time;j++)
		{
			heap.push(make_pair(p[j],p[j].second));
		}
		// if there are no process then set the current time to the arrival of the next earliest process
		if(heap.empty()){
			current_time = p[j].first;
			continue;
		}

		// get the shortest remaining process from the heap
		pair<pair<int,int>,int> curr = heap.top();
		heap.pop();
		if(j == n)
		{
			// if no more processs are going to arrive
			current_time += curr.second;// increment the current time by the remaining time
			turnaround_time += current_time - curr.first.first;// increment the turnaround time by the process's turnaround time
			i++;// Increment number of processes finished
		}
		// Else there are processes yet to arrive, may arrive in between
		else
		{
			if(p[j].first < current_time + curr.second)// If the shortest process does not finish before another one arries
			{	
				// Run this process till the next one arrives
				curr.second -= p[j].first - current_time;// update the time remaining in the process
				current_time = p[j].first;// set the current time to arrival time
				// If the current process still has some time left
				if(curr.second!=0)
					heap.push(curr);// push the prvious process if it remains
				// Process sompleted
				else
				{// increment total turnaround time
					turnaround_time += current_time - curr.first.first;
					i++;// Increment number of processes finished
				}
			}
			else// if the process finishes before another arrives
			{
				current_time += curr.second;// Increment the current time by this processes's remaining time
				turnaround_time += current_time - curr.first.first;//increment total turn around time
				i++;  // Increment number of processes finished
			}
		}

	}

	//Then we return the average
	return (double)turnaround_time/(p.size());
}


// A new compare function for HRRN that return the one with higher Response ratio
bool compare2(pair<pair<int,int>,double> a,pair<pair<int,int>,double> b)
{
	return a.second<b.second;
}

/*
Function to simulate and compute the average turnaround time for the Highest Response Ratio Next algorithm
*/
double hrn_att(vector<pair<int,int> > p)
{
	// Initialise the total turnaround time and current time to 0
	int turnaround_time = 0;

	int current_time = 0;
	// Here, we are considering each process as a ((arrival time, burst time),response ratio) tuple
	priority_queue<pair<pair<int,int>,double>,
					vector<pair<pair<int,int>,double> >,
					bool(*)(pair<pair<int,int>,double>,pair<pair<int,int>,double>)> heap(compare2);

	// A dummy priority queue, can be made into a vector if efficiency is of prime importance
	priority_queue<pair<pair<int,int>,double>,
					vector<pair<pair<int,int>,double> >,
					bool(*)(pair<pair<int,int>,double>,pair<pair<int,int>,double>)> heap_temp(compare2);
	

	int n = p.size();
	int i=0;
	int j=0;
	while(i<n)
	{
		//The first two while loops are used to update the RR of all the processes in the heap
		// For all the processes in the heap, compute the RR, and push into the temporary heap
		while(!heap.empty())
		{
			pair<pair<int,int>,double> curr = heap.top();
			heap.pop();
			curr.second = 1 + (double)(current_time - curr.first.first)/curr.first.second;
			heap_temp.push(curr);
		}
		// Now, heap is empty and heap_temp contains all the processes with computed RR.
		// Push everything back to the heap.
		while(!heap_temp.empty())
		{
			heap.push(heap_temp.top());
			heap_temp.pop();
		}

		// push all the processes arrived till current time
		for(;j<n && p[j].first<=current_time;j++)
		{
			heap.push(make_pair(p[j],1 + 
					(double)(current_time - p[j].first)/p[j].second));
		}

		if(heap.empty() && j<n)// if the heap is empty then shift current time to the arrival time of the next processes
		{	
			current_time = p[j].first;
			continue;
		}

		pair<pair<int,int>,double> curr = heap.top();// get the process with the highest RR
		heap.pop();// Remove it from the queue
		current_time += curr.first.second;// Increment current time
		turnaround_time += current_time - curr.first.first;// Increment the total turnaround time 
		i++;// Increment number of processes finished
	}

	//Then we return the average
	return (double)turnaround_time/(p.size());
}

// A comparison function for the Pre emptive HRRN used for the heap creation
bool compare3(pair<pair<int,int>,pair<int,double> > a,pair<pair<int,int>,pair<int,double> > b)
{
	return a.second.second<b.second.second;
}
/*
	A preemptive version of the Highest Response Ratio Next Algorithm that checks at every second
*/
double pre_hrn_att(vector<pair<int,int> > p)
{
	// Initialise the total turnaround time and current time to 0
	int turnaround_time = 0;

	int current_time = 0;
	// Initialise two queues, similar to non-preemptive version of this algo.
	// Here we consider each process a ((arrival time, burst time),(remaining time, response ratio)) tuple
	priority_queue<pair<pair<int,int>,pair<int,double> >,
					vector<pair<pair<int,int>,pair<int,double> > >,
					bool(*)(pair<pair<int,int>,pair<int,double> >,pair<pair<int,int>,pair<int,double> >)> heap(compare3);
	priority_queue<pair<pair<int,int>,pair<int,double> >,
					vector<pair<pair<int,int>,pair<int,double> > >,
					bool(*)(pair<pair<int,int>,pair<int,double> >,pair<pair<int,int>,pair<int,double> >)> heap_temp(compare3);
	
	// Now we store the arrival times, burst times, remaining time, RR
	int n = p.size();
	int i=0;
	int j=0;
	while(i<n)
	{
		// First two while loops for updating the response ratio
		/*
		Response Ratio(RR) = (Burst Time + Wait time)/(Burst Time)*/
		while(!heap.empty())
		{
			pair<pair<int,int>,pair<int,double> > curr = heap.top();
			heap.pop();

			curr.second.second = (double)(curr.first.second/*Burst time*/+((current_time - curr.first.first)/*Total time from arrival*/
							-(curr.first.second-curr.second.first))/* Time it was executing*/)/curr.first.second;
			heap_temp.push(curr);
		}
		// Copy the values back to the heap
		while(!heap_temp.empty())
		{
			heap.push(heap_temp.top());
			heap_temp.pop();
		}

		// add all the processes till now into the queue
		for(;j<n && p[j].first<=current_time;j++)
		{
			heap.push(make_pair(p[j],make_pair( p[j].second,
					(double)(p[j].second+current_time - p[j].first)/p[j].second)));
		}

		// If heap is empty the fast forward to the arrival of the next process
		if(heap.empty() && j<n)
		{	
			current_time = p[j].first;
			continue;
		}

		pair<pair<int,int>,pair<int, double> > curr = heap.top();// find the process with highest RR
		heap.pop();// remove it from the queue

		current_time +=1;// Increase the current time
		curr.second.first--;// Decrease the remaining time of the process
		if(curr.second.first == 0)// If the processes is over,
		{
			i++;// Increment number of processes finished
			turnaround_time += current_time - curr.first.first;// Increment the turnaround time
		}
		else
		{
			heap.push(curr);// Just puch the processes back. Its RR will be updated at the beginning of the loop in the next iteration
		}
	
	}

	//Then we return the ATT
	return (double)turnaround_time/(p.size());
}
