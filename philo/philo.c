#include "philo.h"

int is_notvalid_number(char *str)
{
	if (!str || *str == '\0')
		return (-1);
	while(*str)
	{
		if (!ft_isdigit(*str))
			return (-1);
		str++;
	}
	return (0);
}

__uint64_t ft_get_time(void)
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL))
		return (-1);
	return ((tv.tv_sec * (__uint64_t)1000) + (tv.tv_usec / 1000));
}
int   ft_initialize_data(t_data *data, int ac, char *av[])
{
	int i;

	i = 0;
	if (is_notvalid_number(av[1]) || is_notvalid_number(av[2]) || 
	is_notvalid_number(av[3]) || is_notvalid_number(av[4]))
		return (-1); 
	data->number_of_philosophers = ft_atoi(av[1]);
	data->time_to_die = ft_atoi(av[2]);
	data->time_to_eat = ft_atoi(av[3]);
	data->time_to_sleep = ft_atoi(av[4]);
	if (ac == 6)
	{
		if (is_notvalid_number(av[5]))
			return (-1);
		data->number_of_times_each_philosopher_must_eat = ft_atoi(av[5]);
	}
	else
		data->number_of_times_each_philosopher_must_eat = -1;
	data->forks = malloc(sizeof(pthread_mutex_t) * data->number_of_philosophers);
	if (!data->forks)
		return (-1);
	while (i < data->number_of_philosophers)
		pthread_mutex_init(&data->forks[i++], NULL);
	pthread_mutex_init(&data->death_mutex, NULL);
	data->someone_died = 0;
	data->timestamp_in_ms = ft_get_time(); //! potential data race
	return (0);
}
int ft_philos(t_data *data)
{
	int i;

	data->ph = malloc(sizeof(t_philo) * data->number_of_philosophers);
	if (!data->ph)
		return(-1);
	i = 0;
	while (i < data->number_of_philosophers)
	{
		data->ph[i].index = i + 1; 
		data->ph[i].left_fork = &data->forks[i];
		data->ph[i].right_fork = &data->forks[(i + 1) % data->number_of_philosophers];
		data->ph[i].data = data;
		data->ph[i].last_time_ate = ft_get_time();
		if (pthread_mutex_init(&data->ph[i].state_mutex, NULL) != 0)
		{
			while (--i > 0)
				pthread_mutex_destroy(&data->ph[i].state_mutex);
			free(data->ph);
			return (-1);
		}
		i++;
	}
	return (0);
}
void *ft_ifphilo_die(void *arg)
{
    t_data		*data;
	int			i;
	__uint64_t	time_now;
	__uint64_t	time_since_last_meal;
	__uint64_t	local_last_time_ate;

	data = (t_data *)arg;
    while (1)
    {
        i = 0;
        while (i < data->number_of_philosophers)
        {
           	pthread_mutex_lock(&data->ph[i].state_mutex);
			local_last_time_ate = data->ph[i].last_time_ate;
           	pthread_mutex_unlock(&data->ph[i].state_mutex);
			time_now = ft_get_time();
			time_since_last_meal = time_now - local_last_time_ate;
            pthread_mutex_lock(&data->death_mutex); //* lock
            if (data->someone_died)
            {
                pthread_mutex_unlock(&data->death_mutex); //^ unlock (if someone already dead)
                return (NULL);
            }

            if (time_since_last_meal >= (__uint64_t)data->time_to_die)
            {
                data->someone_died = 1;
                printf("%lu %d died\n", ft_get_time() - data->timestamp_in_ms, data->ph[i].index);
                pthread_mutex_unlock(&data->death_mutex); //^ unlock (if someone died)
                return (NULL);
            }
            pthread_mutex_unlock(&data->death_mutex); //^ unlock anyway
            i++;
        }
        usleep(500); //? still do not understand sleep
    }
}


int ft_philo_eat(t_philo *philo)
{
	t_data		*data;
	__uint64_t	current_time;

	data = philo->data;
	// line 127-132: we might not need
	pthread_mutex_lock(&data->death_mutex); //* lock->death_mutex (1)
	if (data->someone_died)
	{
		pthread_mutex_unlock(&data->death_mutex); //^ unlock->death_mutex (if someone died)
		return(-1);
	}
	pthread_mutex_unlock(&data->death_mutex); //^ unlock->death_mutex anyway (1)
	pthread_mutex_lock(&philo->state_mutex); //! lock death check to prevent a philo form taken a fork while one is dead
	pthread_mutex_lock(philo->left_fork); //* lock->left_fork
	current_time = ft_get_time();
	printf("%lu %d has taken a fork\n", current_time - data->timestamp_in_ms, philo->index);
	pthread_mutex_lock(philo->right_fork); //* lock->right_fork
	current_time = ft_get_time();
	printf("%lu %d has taken a fork\n", current_time - data->timestamp_in_ms, philo->index);
	pthread_mutex_unlock(&philo->state_mutex); //! unlock 
	pthread_mutex_lock(&data->death_mutex); //* lock->death_mutex (2)
	if (data->someone_died)
	{
		pthread_mutex_unlock(&data->death_mutex); //^ unlock->left_fork - so i can return 
		pthread_mutex_unlock(philo->left_fork); //^ unlock->right_fork - so i can return 
		pthread_mutex_unlock(philo->right_fork); //^ unlock->death_mutex (if someone died)
		return (-1);
	}
	pthread_mutex_lock(&philo->state_mutex);
	philo->last_time_ate = ft_get_time(); // Update the last meal time
	current_time = philo->last_time_ate; // Use this precise time for  the "eating" messege
	pthread_mutex_unlock(&philo->state_mutex);
	printf("%lu %d is eating\n", current_time - data->timestamp_in_ms, philo->index);
	pthread_mutex_unlock(&data->death_mutex); //^ unlock->death_mutex anyway (2)
	usleep((data->time_to_eat * 1000));
	pthread_mutex_unlock(philo->left_fork); //^ unlock->left_fork - philo ate
	pthread_mutex_unlock(philo->right_fork); //^ unlock->right_fork - philo ate
	return (0);
}
void *ft_routine(void *arg)
{
	t_philo *philo = (t_philo *)arg;
	t_data *data = philo->data;
	__uint64_t now;

	// for(int i = 0; i < data->number_of_times_each_philosopher_must_eat; i++)
	if (philo->index % 2 == 0) //! sleep some for some to eat 
		usleep(500);
	while (1)
	{
		pthread_mutex_lock(&data->death_mutex); //* lock->death_mutex - befor start eating 
		if (data->someone_died)
		{
			pthread_mutex_unlock(&data->death_mutex); //^ unlock (if someone died)
			return (NULL);
		}
		pthread_mutex_unlock(&data->death_mutex); //^ unlock anyway
		if (ft_philo_eat(philo))
			return (NULL);
		pthread_mutex_lock(&data->death_mutex); //* lock->death_mutex - brfor sleeping 
		if (data->someone_died)
		{
			pthread_mutex_unlock(&data->death_mutex); //^ unlock (if someone died)
			return (NULL);
		}
		pthread_mutex_unlock(&data->death_mutex); //^ unlock anyway
		printf("%lu %d is sleeping\n", (now = ft_get_time()) - data->timestamp_in_ms, philo->index);
		usleep((data->time_to_sleep * 1000));
		pthread_mutex_lock(&data->death_mutex); //* lock->death_mutex - after sleeping and befor thinking
		if (data->someone_died)
		{
			pthread_mutex_unlock(&data->death_mutex); //^ unlock (if someone died)
			return (NULL);
		}
		pthread_mutex_unlock(&data->death_mutex); //^ unlock anyway
		printf("%lu %d is thinking\n", (now = ft_get_time()) - data->timestamp_in_ms, philo->index);
	}
	return (NULL);
}



int ft_start_simulation(t_data *data)
{
	int i;
	pthread_t monitor;

	i = 0;
	if (ft_philos(data))
		return (-1);
	while (i < data->number_of_philosophers)
	{
		pthread_create(&data->ph[i].th, NULL, ft_routine, &data->ph[i]);
		i++;
	}
	usleep(1000); //! why
	pthread_create(&monitor, NULL, ft_ifphilo_die, (void *)data); //!! creat moniter thread 
	i = 0;
	pthread_join(monitor, NULL);
	while (i < data->number_of_philosophers)
	{
		pthread_join(data->ph[i].th, NULL);
		i++;
	}
	return (0);
}

int main(int ac , char *av[])
{
	t_data data;

	if (ac == 5 || ac == 6)
	{
		if (ft_initialize_data(&data, ac, av))
			return (write(2, "Erorr\n",6), 1);
		if (ft_start_simulation(&data))
			return (write(2, "Erorr\n",6), 1);
	}
	else
		printf("\033[1;31m❌ Wrong number of arguments! ❌\033[0m\n");
}