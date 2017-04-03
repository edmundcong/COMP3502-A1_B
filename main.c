#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int no_of_customers;
int no_of_seats;
int no_of_free_seats;
int no_of_terminals;
int no_of_free_terms;
int customer_arrival_rate;
int terminal_usage_time;
int total_uses = 0; // to see if we're done

// anything you want to ad


void * customer_routine(void *);
void * attendant_routine(void *);

// declare global mutex and condition variables
pthread_mutex_t terminal_mutex; // for decreasing the amount of available terminals
pthread_cond_t terminal_cond; //signal that a terminal is free

pthread_mutex_t seat_mutex; // for decreasing the amount of available terminals
pthread_cond_t seat_cond; //signal that a terminal is free

pthread_mutex_t occupy_mutex; // for decreasing the amount of available terminals
pthread_cond_t occupy_cond; //signal that a terminal is free

// struct for customer
typedef struct customer_data {
    int ID;
    int rate;
} customer_data;

int main(int argc, char ** argv)
{
    //any variables
    pthread_t attendant_thread = malloc(sizeof(pthread_t)); //attendant thread
    int attendant_thread_response;
    int customer_threads_response;

    // ask user to provide the total number of seats & total number of terminals.
    printf("Enter the total number of seats & total number of terminals (int): \n");
    scanf("%d %d", &no_of_seats, &no_of_terminals);

    // ask user to provide the total number of customers, customers arrival rate & terminal usage time.
    printf("Enter the total number of customers, customers arrival rate & terminal usage time (int): \n");
    scanf("%d %d %d", &no_of_customers, &customer_arrival_rate, &terminal_usage_time);

    //Initialize mutexes and condition variable objects
    pthread_mutex_init(&terminal_mutex, NULL);
    pthread_cond_init(&terminal_cond, NULL);

    // anything you want to add
    pthread_t customer_threads[no_of_customers];
    customer_data customer_structs[no_of_customers];
    no_of_free_seats = no_of_seats; // at this point we have only free seats
    no_of_free_terms = no_of_terminals; // at this point we have only free terminals

    // create the attendant thread.
    // needs number of clients and number of free terminals
    attendant_thread_response = pthread_create(&attendant_thread, NULL, attendant_routine, NULL); //farmer routine takes farmer_pace as the arg
    if (attendant_thread_response) {
        printf("ERROR; return code from pthread_create() (attendant) is %d\n", attendant_thread_response);
        exit(-1);
    }

    //create consumer threads according to the arrival rate (in the range between 0 and arrival rate) and
    srand(time(NULL));   // to randomise rate for each customer
    for (int i = 0; i < no_of_customers; i++) {
        sleep((unsigned int) (rand() % customer_arrival_rate)); // can sleep based on the arrival time of clients
        customer_structs[i].ID = i; // the customer id
        customer_structs[i].rate = rand() % terminal_usage_time; // terminal usage rate
        customer_threads_response = pthread_create(&customer_threads[i], NULL, customer_routine, &customer_structs[i]); //farmer routine takes farmer_pace as the arg
        if (customer_threads_response) {
            printf("ERROR; return code from pthread_create() (client) is %d\n", customer_threads_response );
            exit(-1);
        }
    }

    for (int i = 0; i < no_of_customers; i++) {
        pthread_join(customer_threads[i], NULL);
    }

    // terminate the attendant thread
    pthread_cancel(attendant_thread);

    //anything you want to add
    pthread_exit(NULL);
}

void * attendant_routine(void * noargs)
{
    while (true) {
        if ( no_of_free_seats != no_of_seats) //Continue to serve customers.
        {
            printf("Attendant: The number of free seats now is %d. try to find a free terminal.\n", no_of_free_seats);
            if ( no_of_free_terms != 0) { // at least 1 terminal is free
                printf("Attendant: The number of free terminals is %d. There are free terminals now. \n" , no_of_free_terms);
                pthread_mutex_lock(&terminal_mutex); // since we're changing free term variable
                for (int i = 0; i < no_of_free_terms; i++) { // get a thread for each free terminal
                    pthread_cond_signal(&terminal_cond);
                    no_of_free_terms--;
                    printf("Attendant: Assign one terminal to the customer. The number of free terminals is now %d.\n" , no_of_free_terms);
                    // client use terminal
                    sleep(1);
                    no_of_free_terms++;
                    total_uses++;
                }
                pthread_mutex_unlock(&terminal_mutex);
            } else { // wait until terminal is free
                printf("Attendant: The number of free terminal s is %d. All terminals are occupied. \n" , no_of_free_terms);
                pthread_mutex_lock(&occupy_mutex);
                pthread_cond_wait(&occupy_cond, &occupy_mutex);
                printf("Attendant: Call one customer. The number of free seats is now %d.\n", no_of_free_seats );
                pthread_mutex_unlock(&occupy_mutex);
            }
        }
        else if (total_uses == no_of_customers) {
            printf("Attendant: The number of free seats is %d. No customers and I'm waiting. \n", no_of_free_seats);
            pthread_exit(NULL);
        }
    }

}

void * customer_routine(void * args)
{
    customer_data* customer = (customer_data*) args;
    printf("Customer %d arrives.\n", customer->ID);
    while (no_of_free_seats != 0) { // free seats
        pthread_mutex_lock(&seat_mutex);
        no_of_free_seats--;
        printf("Customer %d: I'm lucky to get a free seat from %d.\n", customer->ID, no_of_free_seats);
        pthread_mutex_lock(&terminal_mutex);
        pthread_cond_wait(&terminal_cond, &terminal_mutex); // wait until we can occupy a terminal
        printf("Customer %d: I'm to be served.\n", customer->ID);
        printf("Customer %d: I'm getting a terminal now.\n", customer->ID);
        no_of_free_seats++;
        total_uses++;
        sleep(1);
        pthread_cond_signal(&occupy_cond); // signal that we're done with the terminal
        pthread_mutex_unlock(&terminal_mutex);
        printf("Customer %d: I'm finished using the terminal and leaving.\n", customer->ID);
        pthread_mutex_unlock(&seat_mutex);
        pthread_exit(NULL); // no free seats so leave
    }
    printf("Customer %d: oh no! all seats have been taken and I'll leave now!\n", customer->ID);
    pthread_exit(NULL); // no free seats so leave
}