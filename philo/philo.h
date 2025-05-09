#ifndef PHILO_H
#define PHILO_H

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>


typedef struct s_philo t_philo;

typedef struct s_data
{
    int number_of_philosophers;
    int time_to_die;
    int time_to_eat;
    int time_to_sleep;
    int number_of_times_each_philosopher_must_eat;
    __uint64_t timestamp_in_ms;
    int someone_died;
    pthread_mutex_t *forks;
    pthread_mutex_t death_mutex;
    t_philo *ph;


}   t_data;

typedef struct s_philo
{
    pthread_t    th;
    int         index;
    __uint64_t  last_time_ate;
    __uint64_t  time_now;
    __uint64_t  time_since_last_meal;
    pthread_mutex_t  *left_fork;
    pthread_mutex_t  *right_fork;
	pthread_mutex_t	 state_mutex; //% just added
    t_data      *data;

}   t_philo;




long	ft_atoi(const char *str);
int ft_isdigit(int c);

#endif