#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// process control block (PCB)
struct pcb
{
  unsigned int pid;
  char pname[20];
  unsigned int ptimeleft;
  unsigned int ptimearrival;
  unsigned int turnaroundtime;
  unsigned int ptimecompleted;
};

typedef struct pcb pcb;

// queue node
struct dlq_node
{
  struct dlq_node *pfwd;
  struct dlq_node *pbck;
  struct pcb *data;
};

typedef struct dlq_node dlq_node;

// queue
struct dlq
{
  struct dlq_node *head;
  struct dlq_node *tail;
};

typedef struct dlq dlq;

int Total_Process;

// function to add a pcb to a new queue node
dlq_node *get_new_node(pcb *ndata)
{
  if (!ndata)
    return NULL;

  dlq_node *new = malloc(sizeof(dlq_node));
  if (!new)
  {
    fprintf(stderr, "Error: allocating memory\n");
    exit(1);
  }

  new->pfwd = new->pbck = NULL;
  new->data = ndata;
  return new;
}

// function to add a node to the tail of queue
void add_to_tail(dlq *q, dlq_node *new)
{
  if (!new)
    return;

  if (q->head == NULL)
  {
    if (q->tail != NULL)
    {
      fprintf(stderr, "DLList inconsitent.\n");
      exit(1);
    }
    q->head = new;
    q->tail = q->head;
  }
  else
  {
    new->pfwd = q->tail;
    new->pbck = NULL;
    new->pfwd->pbck = new;
    q->tail = new;
  }
}

// function to remove a node from the head of queue
dlq_node *remove_from_head(dlq *const q)
{
  if (q->head == NULL)
  { // empty
    if (q->tail != NULL)
    {
      fprintf(stderr, "DLList inconsitent.\n");
      exit(1);
    }
    return NULL;
  }
  else if (q->head == q->tail)
  { // one element
    if (q->head->pbck != NULL || q->tail->pfwd != NULL)
    {
      fprintf(stderr, "DLList inconsitent.\n");
      exit(1);
    }

    dlq_node *p = q->head;
    q->head = NULL;
    q->tail = NULL;

    p->pfwd = p->pbck = NULL;
    return p;
  }
  else
  { // normal
    dlq_node *p = q->head;
    q->head = q->head->pbck;
    q->head->pfwd = NULL;

    p->pfwd = p->pbck = NULL;
    return p;
  }
}

// function to print our queue
void print_q(const dlq *q)
{
  dlq_node *n = q->head;
  if (n == NULL)
    return;

  while (n)
  {
    printf("%s(%d),", n->data->pname, n->data->ptimeleft);
    n = n->pbck;
  }
}

// function to check if the queue is empty
int is_empty(const dlq *q)
{
  if (q->head == NULL && q->tail == NULL)
    return 1;
  else if (q->head != NULL && q->tail != NULL)
    return 0;
  else
  {
    fprintf(stderr, "Error: DLL queue is inconsistent.");
    exit(1);
  }
}

// function to sort the queue on completion time
void sort_by_timetocompletion(const dlq *q)
{
  // bubble sort
  dlq_node *start = q->tail;
  dlq_node *end = q->head;

  while (start != end)
  {
    dlq_node *node = start;
    dlq_node *next = node->pfwd;

    while (next != NULL)
    {
      if (node->data->ptimeleft < next->data->ptimeleft)
      {
        // do a swap
        pcb *temp = node->data;
        node->data = next->data;
        next->data = temp;
      }
      node = next;
      next = node->pfwd;
    }
    end = end->pbck;
  }
}

// function to sort the queue on arrival time
void sort_by_arrival_time(const dlq *q)
{
  // bubble sort
  dlq_node *start = q->tail;
  dlq_node *end = q->head;

  while (start != end)
  {
    dlq_node *node = start;
    dlq_node *next = node->pfwd;

    while (next != NULL)
    {
      if (node->data->ptimearrival < next->data->ptimearrival)
      {
        // do a swap
        pcb *temp = node->data;
        node->data = next->data;
        next->data = temp;
      }
      node = next;
      next = node->pfwd;
    }
    end = end->pbck;
  }
}

// function to tokenize the one row of data
pcb *tokenize_pdata(char *buf)
{
  pcb *p = (pcb *)malloc(sizeof(pcb));
  if (!p)
  {
    fprintf(stderr, "Error: allocating memory.\n");
    exit(1);
  }

  char *token = strtok(buf, ":\n");
  if (!token)
  {
    fprintf(stderr, "Error: Expecting token pname\n");
    exit(1);
  }
  strcpy(p->pname, token);

  token = strtok(NULL, ":\n");
  if (!token)
  {
    fprintf(stderr, "Error: Expecting token pid\n");
    exit(1);
  }
  p->pid = atoi(token);

  token = strtok(NULL, ":\n");
  if (!token)
  {
    fprintf(stderr, "Error: Expecting token duration\n");
    exit(1);
  }

  p->ptimeleft = atoi(token);

  token = strtok(NULL, ":\n");
  if (!token)
  {
    fprintf(stderr, "Error: Expecting token arrival time\n");
    exit(1);
  }
  p->ptimearrival = atoi(token);

  token = strtok(NULL, ":\n");
  if (token)
  {
    fprintf(stderr, "Error: Oh, what've you got at the end of the line?\n");
    exit(1);
  }

  return p;
}

// implement the FIFO scheduling code
void sched_FIFO(dlq *const p_fq, int *p_time)
{
  // Define variables to store statistics
  float total_throughput = 0.0;
  float total_turnaround_time = 0.0;
  float total_response_time = 0.0;
  int completed_processes = 0;

  while (!is_empty(p_fq))
  {
    (*p_time)++;

    // Check arrival time
    if (p_fq->head->data->ptimearrival > *p_time)
    {
      printf("%d:idle:empty:\n", *p_time);
      continue;
    }

    dlq_node *current = remove_from_head(p_fq);
    pcb *process = current->data;

    printf("%d:%s:", *p_time, process->pname);

    // Print ready queue
    print_q(p_fq);
    printf(":\n");

    process->ptimeleft--;

    if (process->ptimeleft > 0)
    {
      add_to_tail(p_fq, current);
    }
    else
    {
      process->ptimecompleted = *p_time; // Set completion time

      // Calculate and update metrics for this process
      float turnaround_time = (float)(process->ptimecompleted - process->ptimearrival);
      float response_time = (float)(process->ptimecompleted - process->ptimearrival - process->ptimeleft);
      total_turnaround_time += turnaround_time;
      total_response_time += response_time;
      total_throughput += 1.0;
      completed_processes++;

      // Print completion message
      printf("%s completed (TAT: %.2f, RT: %.2f)\n", process->pname, turnaround_time, response_time);

      free(current);
    }
  }

  // Calculate and print overall statistics
  float avg_throughput = total_throughput / completed_processes;
  float avg_turnaround_time = total_turnaround_time / completed_processes;
  float avg_response_time = total_response_time / completed_processes;

  printf("Average Throughput: %.2f\n", avg_throughput);
  printf("Average Turnaround Time: %.2f\n", avg_turnaround_time);
  printf("Average Response Time: %.2f\n", avg_response_time);
}

// implement the SJF scheduling code
void sched_SJF(dlq *const p_fq, int *p_time)
{
  // Define variables to store statistics
  float total_throughput = 0.0;
  float total_turnaround_time = 0.0;
  float total_response_time = 0.0;
  int completed_processes = 0;

  while (!is_empty(p_fq))
  {
    (*p_time)++;

    // Check arrival time
    if (p_fq->head->data->ptimearrival > *p_time)
    {
      printf("%d:idle:empty:\n", *p_time);
      continue;
    }

    // Find process with shortest time left
    dlq_node *shortest = NULL;
    dlq_node *current = p_fq->head;

    while (current != NULL)
    {
      if (shortest == NULL || current->data->ptimeleft < shortest->data->ptimeleft)
      {
        shortest = current;
      }
      current = current->pfwd;
    }

    // Remove the shortest process
    dlq_node *next = remove_from_head(p_fq);
    pcb *process = next->data;

    printf("%d:%s:", *p_time, process->pname);

    // Print ready queue
    print_q(p_fq);
    printf(":\n");

    process->ptimeleft--;

    if (process->ptimeleft > 0)
    {
      add_to_tail(p_fq, next);
    }
    else
    {
      process->ptimecompleted = *p_time; // Set completion time

      // Calculate and update metrics for this process
      float turnaround_time = (float)(process->ptimecompleted - process->ptimearrival);
      float response_time = (float)(process->ptimecompleted - process->ptimearrival - process->ptimeleft);
      total_turnaround_time += turnaround_time;
      total_response_time += response_time;
      total_throughput += 1.0;
      completed_processes++;

      // Print completion message
      printf("%s completed (TAT: %.2f, RT: %.2f)\n", process->pname, turnaround_time, response_time);

      free(next);
    }
  }

  // Calculate and print overall statistics
  float avg_throughput = total_throughput / completed_processes;
  float avg_turnaround_time = total_turnaround_time / completed_processes;
  float avg_response_time = total_response_time / completed_processes;

  printf("Average Throughput: %.2f\n", avg_throughput);
  printf("Average Turnaround Time: %.2f\n", avg_turnaround_time);
  printf("Average Response Time: %.2f\n", avg_response_time);
}

// implement the STCF scheduling code
void sched_STCF(dlq *const p_fq, int *p_time)
{
  // Define variables to store statistics
  float total_throughput = 0.0;
  float total_turnaround_time = 0.0;
  float total_response_time = 0.0;
  int completed_processes = 0;

  while (!is_empty(p_fq))
  {
    (*p_time)++;

    // Check arrival time
    if (p_fq->head->data->ptimearrival > *p_time)
    {
      printf("%d:idle:empty:\n", *p_time);
      continue;
    }

    // Sort by shortest time left
    sort_by_timetocompletion(p_fq);

    // Get process with shortest time left
    dlq_node *next = remove_from_head(p_fq);
    pcb *process = next->data;

    printf("%d:%s:", *p_time, process->pname);

    // Print ready queue
    print_q(p_fq);
    printf(":\n");

    process->ptimeleft--;

    if (process->ptimeleft > 0)
    {
      add_to_tail(p_fq, next);
    }
    else
    {
      process->ptimecompleted = *p_time; // Set completion time

      // Calculate and update metrics for this process
      float turnaround_time = (float)(process->ptimecompleted - process->ptimearrival);
      float response_time = (float)(process->ptimecompleted - process->ptimearrival - process->ptimeleft);
      total_turnaround_time += turnaround_time;
      total_response_time += response_time;
      total_throughput += 1.0;
      completed_processes++;

      // Print completion message
      printf("%s completed (TAT: %.2f, RT: %.2f)\n", process->pname, turnaround_time, response_time);

      free(next);
    }
  }

  // Calculate and print overall statistics
  float avg_throughput = total_throughput / completed_processes;
  float avg_turnaround_time = total_turnaround_time / completed_processes;
  float avg_response_time = total_response_time / completed_processes;

  printf("Average Throughput: %.2f\n", avg_throughput);
  printf("Average Turnaround Time: %.2f\n", avg_turnaround_time);
  printf("Average Response Time: %.2f\n", avg_response_time);
}

void sched_RR(dlq *p_fq, int *p_time)
{
  int quantum = 1;
  float total_throughput = 0.0;
  float total_turnaround_time = 0.0;
  float total_response_time = 0.0;
  int completed_processes = 0;

  while (!is_empty(p_fq))
  {
    (*p_time)++;

    pcb *next = p_fq->head->data;
    if (next->ptimearrival > *p_time)
    {
      printf("%d:idle:empty:\n", *p_time);
      continue;
    }

    dlq_node *current = remove_from_head(p_fq);
    pcb *process = current->data;

    printf("%d:%s:", *p_time, process->pname);
    print_q(p_fq);
    printf(":\n");

    if (process->ptimeleft > quantum)
    {
      process->ptimeleft -= quantum;
    }
    else
    {
      process->ptimeleft = 0;
      process->ptimecompleted = *p_time; // Set completion time

      // Calculate and update metrics for this process
      float turnaround_time = (float)(process->ptimecompleted - process->ptimearrival);
      float response_time = (float)(process->ptimecompleted - process->ptimearrival - process->ptimeleft);
      total_turnaround_time += turnaround_time;
      total_response_time += response_time;
      total_throughput += 1.0;
      completed_processes++;

      // Print completion message
      printf("%s completed (TAT: %.2f, RT: %.2f)\n", process->pname, turnaround_time, response_time);

      free(current);
    }

    if (process->ptimeleft > 0)
    {
      add_to_tail(p_fq, current);
    }
  }

  // Calculate and print overall statistics
  float avg_throughput = total_throughput / completed_processes;
  float avg_turnaround_time = total_turnaround_time / completed_processes;
  float avg_response_time = total_response_time / completed_processes;

  printf("Average Throughput: %.2f\n", avg_throughput);
  printf("Average Turnaround Time: %.2f\n", avg_turnaround_time);
  printf("Average Response Time: %.2f\n", avg_response_time);
}

int main()
{
  /* Enter your code here. Read input from STDIN. Print output to STDOUT */
  int N = 0;
  char tech[20] = {'\0'};
  char buffer[100] = {'\0'};
  scanf("%d", &N);
  scanf("%s", tech);

  dlq queue;
  queue.head = NULL;
  queue.tail = NULL;
  for (int i = 0; i < N; ++i)
  {
    scanf("%s\n", buffer);
    pcb *p = tokenize_pdata(buffer);
    add_to_tail(&queue, get_new_node(p));
  }
  // print_q(&queue);
  unsigned int system_time = 0;
  sort_by_arrival_time(&queue);

  // run scheduler
  if (!strncmp(tech, "FIFO", 4))
    sched_FIFO(&queue, &system_time);
  else if (!strncmp(tech, "SJF", 3))
    sched_SJF(&queue, &system_time);
  else if (!strncmp(tech, "STCF", 4))
    sched_STCF(&queue, &system_time);
  else if (!strncmp(tech, "RR", 2))
    sched_RR(&queue, &system_time);
  else
    fprintf(stderr, "Error: unknown POLICY\n");
  return 0;
}