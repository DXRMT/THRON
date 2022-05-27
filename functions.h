#define UP 'w'
#define DOWN 's'
#define LEFT 'a'
#define RIGHT 'd'

#define RED 0x00FF0000
#define BLUE 0x00F3F200
#define PURPLE 0x00F1F2F3

int work_flag = 1;
int start_flag = 0;
int wait_flag = 0;
char count_step = 0;

struct args_keys
{
  int sockfd;
  char* ptr_direct;
  char* ptr_is_ready_player;
  struct sockaddr_in* ptr_p2_addr;
  pthread_mutex_t* ptr_mtx;
};

struct termios stored_settings;

/**
Map of functions:
    void set_keypress(void);
    void reset_keypress(void);
    int get_local_ip(unsigned long addr_c);
    void move_car(uint32_t** ptr_car, char direct, int scr_xres);
    char set_opposite_direct(char direct, char direct_prev, char* ptr_opposite_direct);
    void invert_four_bytes(char *ptr);
    char is_cross(uint32_t * car_p1, uint32_t* car_p2, char direct_p1, char direct_p2, int scr_xres);
    void draw_area(uint32_t *ptr, int xres, int yres, int scr_xres);
    int draw_car(uint32_t* ptr, char direction, uint32_t color, int scr_xres);
    void delete_car(uint32_t* ptr, char direction, int scr_xres, uint32_t background_color);
    void control_thread_nsync(struct args_keys* args);
    void interaction_thread_nsync(struct args_keys* args);
    void control_thread_sync(struct args_keys* args);
    void interaction_thread_sync(struct args_keys* args);
    int handler(int none);
**/

void set_keypress(void)
{
    struct termios new_settings;

    tcgetattr(0,&stored_settings);

    new_settings = stored_settings;

    new_settings.c_lflag &= (~ICANON & ~ECHO);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;

    tcsetattr(0,TCSANOW,&new_settings);
    return;
}

void reset_keypress(void)
{
    tcsetattr(0,TCSANOW,&stored_settings);
	return;
}


int get_local_ip(unsigned long addr_c)
{
    struct ifaddrs *ifaddr;
    unsigned long addr_s;
    int mask;
    if (getifaddrs(&ifaddr) == -1) 
    {
        perror("getifaddrs");
        return 0;
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL;
                ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) 
        {
            addr_s = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
            if(addr_s != 0)
            {
                mask = ((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr.s_addr; 
                if((addr_s&mask) == (addr_c&mask))
                {
                    return addr_s;
                }
            }
        }
    }
    return 0;
}

void move_car(uint32_t** ptr_car, char direct, int scr_xres)
{
  switch(direct)
  {
    case UP:
      {
        *ptr_car -= scr_xres;
        break;
      }
      case DOWN:
      {
        *ptr_car += scr_xres;
        break;
      }
      case LEFT:
      {
        *ptr_car -= 1;
        break;
      }
      case RIGHT:
      {
        *ptr_car += 1;
        break;
      }
  }
}

char set_opposite_direct(char direct, char direct_prev, char* ptr_opposite_direct)
{
    switch(direct)
    {
      case UP:
      {
        *ptr_opposite_direct = DOWN;
        if( direct_prev == LEFT || direct_prev == RIGHT )
            return 1;
        else
            return 0;
      }
      case DOWN:
      {
        *ptr_opposite_direct = UP;  
        if( direct_prev == LEFT || direct_prev == RIGHT )
            return 1;
        else 
            return 0;
        break;
      }
      case LEFT:
      {
        *ptr_opposite_direct = RIGHT;  
        if( direct_prev == UP || direct_prev == DOWN )
            return 1;
        else 
            return 0;
      }
      case RIGHT:
      {
        *ptr_opposite_direct = LEFT;  
        if( direct_prev == UP || direct_prev == DOWN )
            return 1;
        else 
            return 0;
      }
    }
}

void invert_four_bytes(char *ptr)
{
    char tmp=ptr[0];
    ptr[0]=ptr[3];
    ptr[3]=tmp;
    
    tmp=ptr[1];
    ptr[1]=ptr[2];
    ptr[2]=tmp;
}

char is_cross(uint32_t * car_p1, uint32_t* car_p2, char direct_p1, char direct_p2, int scr_xres)
{
    uint32_t* copy_car_p1[40];
    int index = 0;
    switch (direct_p1)
    {
        case UP:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                   copy_car_p1[index] = car_p1 + j + i*scr_xres;
                   index ++;
                }
            }
            break;
        case DOWN:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                   copy_car_p1[index] = car_p1 + j + i*scr_xres;
                   index ++;
                }
            }
            break;
        case LEFT:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                   copy_car_p1[index] = car_p1 + i + j*scr_xres;
                   index ++;
                }
            }
            break;
        case RIGHT:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                   copy_car_p1[index] = car_p1 + i + j*scr_xres;
                   index ++;
                }
            }
            break;
    }  
    index = 0;
    switch (direct_p2)
    {
        case UP:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                  for(int indx = 0; indx < 40; indx++)
                  {
                   uint32_t* result = car_p2 + j + i*scr_xres;
                   if(copy_car_p1[indx] == result)
                   {
                     return 1;
                   }
                  }
                 }
            }
            break;
        case DOWN:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                  for(int indx = 0; indx < 40; indx++)
                  {
                   uint32_t* result = car_p2 + j + i*scr_xres;
                   if(copy_car_p1[indx] == result)
                   {
                     return 1;
                   }
                  }
                }
            }
            break;
        case LEFT:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                  for(int indx = 0; indx < 40; indx++)
                  {
                   uint32_t* result = car_p2 + i + j*scr_xres;
                   if(copy_car_p1[indx] == result)
                   {
                     return 1;
                   }
                  }
                }
            }
            break;
        case RIGHT:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                  for(int indx = 0; indx < 40; indx++)
                  {
                   uint32_t* result = car_p2 + i + j*scr_xres;
                   if(copy_car_p1[indx] == result)
                   {
                     return 1;
                   }
                  }
                }
            }
            break;
    }
    return 0;
}


void draw_area(uint32_t *ptr, int xres, int yres, int scr_xres)
{
  uint32_t color = (count_step%2 == 1) ? RED : BLUE;
  for(int i = 0; i<xres+2; i++)
  {
      ptr[i] = color;
      ptr[i+(yres+1)*scr_xres] = color;
  }
  for(int i = 1; i<=yres; i++)
  {
      ptr[i*scr_xres] = color;
      ptr[i*scr_xres+(xres+1)] = color;
  }
}

int draw_car(uint32_t* ptr, char direction, uint32_t color, int scr_xres)
{
    int was_overlap = 0;
    switch (direction)
    {
        case UP:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                    if( ptr[j+i*scr_xres] == RED || 
                        ptr[j+i*scr_xres] == BLUE)
                        was_overlap = 1;
                    ptr[j+i*scr_xres] = color;
                }
            }
            break;
        case DOWN:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                    if(  ptr[j+i*scr_xres] == RED || 
                        ptr[j+i*scr_xres] == BLUE)
                        was_overlap = 1;
                    ptr[j+i*scr_xres] = color;
                }
            }
            break;
        case LEFT:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                    if( ptr[i+j*scr_xres] == RED || 
                        ptr[i+j*scr_xres] == BLUE)
                        was_overlap = 1;
                    ptr[i+j*scr_xres] = color;
                }
            }
            break;
        case RIGHT:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                    if( ptr[i+j*scr_xres] == RED || 
                        ptr[i+j*scr_xres] == BLUE)
                        was_overlap = 1;
                    ptr[i+j*scr_xres] = color;
                }
            }
            break;
    }
    return was_overlap;
}

void delete_car(uint32_t* ptr, char direction, int scr_xres, uint32_t background_color)
{
    switch (direction)
    {
        case UP:
        {
            for(int i = 0; i>-8; i--)
            {
                ptr[-2+i*scr_xres] = background_color;
                ptr[-1+i*scr_xres] = background_color;
                ptr[i*scr_xres] = background_color;
                ptr[1+i*scr_xres] = background_color;
                ptr[2+i*scr_xres] = background_color;
            }
            break;
        }
        case DOWN:
        {
            for(int i = 0; i<8; i++)
            {
                ptr[-2+i*scr_xres] = background_color;
                ptr[-1+i*scr_xres] = background_color;
                ptr[i*scr_xres] = background_color;
                ptr[1+i*scr_xres] = background_color;
                ptr[2+i*scr_xres] = background_color;
            }
            break;
        }
        case LEFT:
        {
            for(int i = 0; i>-8; i--)
            {
                ptr[-2*scr_xres+i] = background_color;
                ptr[-1*scr_xres+i] = background_color;
                ptr[i] = background_color;
                ptr[scr_xres+i] = background_color;
                ptr[2*scr_xres+i] = background_color;
            }
            break;
        }
        case RIGHT:
        {
            for(int i = 0; i<8; i++)
            {
                ptr[-2*scr_xres+i] = background_color;
                ptr[-1*scr_xres+i] = background_color;
                ptr[i] = background_color;
                ptr[scr_xres+i] = background_color;
                ptr[2*scr_xres+i] = background_color;         
            }
            break;
        }
    }
}

void control_thread_nsync(struct args_keys* args)
{
  int sockfd = args->sockfd;
  char* ptr_direct = args->ptr_direct;
  pthread_mutex_t* ptr_mtx = args->ptr_mtx;
  struct sockaddr* ptr_p2_addr = (struct sockaddr*)args->ptr_p2_addr;
  int len_sockaddr = sizeof(*ptr_p2_addr);
  char direction;
  
  direction = getchar();
  *(args->ptr_is_ready_player) = 1;
  sendto(sockfd, &direction, 1, 0, ptr_p2_addr, len_sockaddr);

  direction = 0;
  //wait start game
  while( start_flag != 1 )
  {
      usleep(1);
      sendto(sockfd, &direction, 1, 0, ptr_p2_addr, len_sockaddr);
  }
  tcflush(0, TCIFLUSH);
  while( direction != 'q' && work_flag )
  {
    direction = getchar();
    if(direction == UP || direction == DOWN || direction == LEFT || direction == RIGHT)
    {
      pthread_mutex_lock(ptr_mtx);
      *ptr_direct = direction; 
      sendto(sockfd, &direction, 1, 0, ptr_p2_addr, len_sockaddr);
      pthread_mutex_unlock(ptr_mtx);
    }
  }
  work_flag = 0;
}

void interaction_thread_nsync(struct args_keys* args)
{
  int sockfd = args->sockfd;
  char* ptr_direct = args->ptr_direct;
  pthread_mutex_t* ptr_mtx = args->ptr_mtx;
  struct sockaddr* ptr_p2_addr = (struct sockaddr*)args->ptr_p2_addr;
  int len_sockaddr = sizeof(*ptr_p2_addr);
  char direction;

  recvfrom(sockfd, &direction, 1, 0, ptr_p2_addr, &len_sockaddr);
  *(args->ptr_is_ready_player) = 1;
  
  //wait start game
  while(start_flag != 1)
  {
      usleep(1);
  }

  while(work_flag)
  {
    recvfrom(sockfd, &direction, 1, 0, ptr_p2_addr, &len_sockaddr);
    if(direction == UP || direction == DOWN || direction == LEFT || direction == RIGHT)
    {
      pthread_mutex_lock(ptr_mtx);
      *ptr_direct = direction; 
      pthread_mutex_unlock(ptr_mtx);
    }
  }
}

void control_thread_sync(struct args_keys* args)
{
  int sockfd = args->sockfd;
  char* ptr_direct = args->ptr_direct;
  pthread_mutex_t* ptr_mtx = args->ptr_mtx;
  struct sockaddr* ptr_p2_addr = (struct sockaddr*)args->ptr_p2_addr;
  int len_sockaddr = sizeof(*ptr_p2_addr);
  char direction = 0;
  
  getchar();
  *(args->ptr_is_ready_player) = 1;
  sendto(sockfd, &direction, 1, 0, ptr_p2_addr, len_sockaddr);

  direction = 0;
  //wait start game
  while( start_flag != 1 )
  {
      usleep(1);
      sendto(sockfd, &direction, 1, 0, ptr_p2_addr, len_sockaddr);
  }
  tcflush(0, TCIFLUSH);
  while( direction != 'q' && work_flag )
  {
    direction = getchar();
    if(direction == UP || direction == DOWN || direction == LEFT || direction == RIGHT)
    {
      pthread_mutex_lock(ptr_mtx);
      *ptr_direct = direction; 
      pthread_mutex_unlock(ptr_mtx);
    }
  }
  work_flag = 0;
}

void interaction_thread_sync(struct args_keys* args)
{
  int sockfd = args->sockfd;
  char* ptr_direct = args->ptr_direct;
  pthread_mutex_t* ptr_mtx = args->ptr_mtx;
  struct sockaddr* ptr_p2_addr = (struct sockaddr*)args->ptr_p2_addr;
  int len_sockaddr = sizeof(*ptr_p2_addr);
  char direction;

  recvfrom(sockfd, &direction, 1, 0, ptr_p2_addr, &len_sockaddr);
  *(args->ptr_is_ready_player) = 1;
  
  //wait start game
  while(start_flag != 1)
  {
      usleep(1);
  }

  while(work_flag)
  {
    recvfrom(sockfd, &direction, 1, 0, ptr_p2_addr, &len_sockaddr);
    direction -= count_step % 2;
    if((direction == UP || direction == DOWN 
                || direction == LEFT || direction == RIGHT) && wait_flag)
    {
      *ptr_direct = direction;
      wait_flag = 0;
    }
  }
}

int handler(int none)
{
    exit(0);
}
