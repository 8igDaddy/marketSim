/*
 *      A Stock Market Simulator
 *      by Pavlos Vougiouklis
 *
 *	Based upon the pc.c producer-consumer demo by Andrae Muys
 *      and the Stock Market Simulator Skeleton by Nikos Pitsianis
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>	
#include <time.h>	
#include <sys/time.h>
#include <string.h>
#include <unistd.h>		
#include <pthread.h>

#define QUEUESIZE 15000

typedef struct {
  long id,oldid;
  long timestamp;
  int vol;
  int price1, price2;
  char action, type;
} order;

void *Prod (void *q);
void *Cons (void *q);

void *Market();
void *MarketBuy();
void *MarketSell();
void *LimitBuy();
void *LimitSell();
void *Cancel();

order makeOrder();
inline long getTimestamp();
void dispOrder (order ord);

int currentPriceX10 = 1000;

struct timeval startwtime, endwtime;

typedef struct {
  order item[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit (void);

typedef struct {
  order buyMarketOrder;
  order sellMarketOrder;
  order buyLimitOrder;
  order sellLimitOrder;

  int marketBuyer;
  int marketSeller;
  int limitBuyer;
  int limitSeller;

  pthread_mutex_t *mutBuyer, *mutSeller;

  pthread_cond_t *conBuyer, *conSeller;
  pthread_cond_t *buyMarketTransaction;
  pthread_cond_t *sellMarketTransaction;
  pthread_cond_t *buyLimitTransaction;
  pthread_cond_t *sellLimitTransaction;
} transaction;

transaction *transactionInit();

void queueAdd (queue *q, order ord);
void queueDel (queue *q, order *ord);
void queueDelete (queue *q);

void orderAdd (int flag, order ord);

queue *buyMarketOrder, *sellMarketOrder;
queue *buyLimitOrder, *sellLimitOrder;
queue *cancelOrder;

transaction *t;

//FILE *infile;



// ****************************************************************
int main() {

  // reset number generator seed
  // srand(time(NULL) + getpid());
  srand(0); // to get the same sequence

  // start the time for timestamps
  gettimeofday (&startwtime, NULL);

  pthread_t prod, cons;
  queue *q = queueInit();
  
  pthread_create(&prod, NULL, Prod, q);
  pthread_create(&cons, NULL, Cons, q);

  pthread_t market;
  pthread_t marketBuy, marketSell;
  pthread_t limitBuy, limitSell;
  pthread_t cancel;

  buyMarketOrder = queueInit();
  sellMarketOrder = queueInit();
  buyLimitOrder = queueInit();
  sellLimitOrder = queueInit();
  cancelOrder = queueInit();

  t = transactionInit();

  pthread_create (&market, NULL, Market, 0);
  pthread_create (&marketBuy, NULL, MarketBuy, 0);
  pthread_create (&marketSell, NULL, MarketSell,0);
  pthread_create (&limitBuy, NULL, LimitBuy, 0);
  pthread_create (&limitSell, NULL, LimitSell, 0);
  pthread_create (&cancel, NULL, Cancel, 0);
  
  pthread_join (market, NULL);
  pthread_join (marketBuy, NULL);
  pthread_join (marketSell, NULL);
  pthread_join (limitBuy, NULL);
  pthread_join (limitSell, NULL);
  pthread_join (cancel, NULL);

  queueDelete (buyMarketOrder);
  queueDelete (sellMarketOrder);
  queueDelete (buyLimitOrder);
  queueDelete (sellLimitOrder);
  queueDelete (cancelOrder);

  pthread_exit (NULL);
  // I actually do not expect them to ever terminate
  pthread_join (prod, NULL);
  pthread_join (cons, NULL);

  pthread_exit (NULL);
}



// ****************************************************************
void *Prod (void *arg) {
  queue *q = (queue *) arg;
  int i;
  
  while (1) {
    pthread_mutex_lock (q->mut);
    while (q->full) {
     // printf ("*** Incoming Order Queue is EMPTY.\n"); fflush(stdout);
      pthread_cond_wait (q->notFull, q->mut);
    }
    queueAdd (q, makeOrder());
    pthread_mutex_unlock (q->mut);
    pthread_cond_signal (q->notEmpty);

  }
}



// ****************************************************************
void *Cons (void *arg) {
  queue *q = (queue *) arg;
  order ord;

  while (1) {
    pthread_mutex_lock (q->mut);
    while (q->empty) {
     // printf ("*** Incoming Order Queue is EMPTY.\n"); fflush(stdout);
      pthread_cond_wait (q->notEmpty, q->mut);
    }
    queueDel (q, &ord);
    pthread_mutex_unlock (q->mut);
    pthread_cond_signal (q->notFull);

    // Order type
    switch (ord.type) {

      case 'M':                     // Market order
        if (ord.action == 'B')
          orderAdd (0, ord);
        else
          orderAdd (1, ord); 
        break;

      case 'L':                     // Limit order
        if (ord.action == 'B')
          orderAdd (2, ord);
        else 
          orderAdd (3, ord);
        break;

      default:                      // Cancel order
        orderAdd (4, ord);
        break;
    
    }

    // YOUR CODE IS CALLED FROM HERE
    // Process that order!
    //printf ("Processing at time %8d : ", getTimestamp());
    //dispOrder(ord); fflush(stdout);

  }
}



//**********************************************************
order makeOrder() {

  static count = 0;

  int magnitude = 10; 
  int waitmsec;

  order ord;

  // wait for a random amount of time in useconds
  waitmsec = ((double)rand()/(double)RAND_MAX * magnitude);
  usleep(waitmsec*1000);
  
  ord.id = count++;
  ord.timestamp = getTimestamp();

  // Buy or Sell
  ord.action = ((double)rand()/(double)RAND_MAX <= 0.53) ? 'B' : 'S';

  // Order type
  double u2 = ((double)rand()/(double)RAND_MAX);
  if (u2 < 0.45){ 
    ord.type = 'M';                 // Market order
    ord.vol = (1 + rand()%50)*100;

  }else if (0.45 <= u2 && u2 < 0.9){
    ord.type = 'L';                 // Limit order
    ord.vol = (1 + rand()%50)*100;
    ord.price1 = currentPriceX10 + 10*(0.5 -((double)rand()/(double)RAND_MAX));
    
  }else if (0.9 <= u2){
    ord.type = 'C';                 // Cancel order
    ord.oldid = ((double)rand()/(double)RAND_MAX)*count;
  }

  //dispOrder(ord);
  
  return (ord);
}



// ****************************************************************
inline long getTimestamp() {

  gettimeofday(&endwtime, NULL);

  return((double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		  + endwtime.tv_sec - startwtime.tv_sec)*1000);
}



// ****************************************************************
void dispOrder (order ord) {

  printf("%ld ", ord.id);
  printf("%ld ", ord.timestamp);  
  switch( ord.type ) {
  case 'M':
    printf("%c ", ord.action);
    printf("Market (%4d)        ", ord.vol); break;
  case 'L':
    printf("%c ", ord.action);
    printf("Limit  (%4d,%5.1f) ", ord.vol, (float) ord.price1/10.0); break;
  case 'C':
    printf("* Cancel  %ld        ", ord.oldid); break;
  default : break;
  }
  printf("\n");
}



// ****************************************************************
queue *queueInit (void) {
  queue *q;

  q = (queue *) malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}



// ****************************************************************
void queueAdd (queue *q, order in){
  q->item[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}


// ****************************************************************
void queuePush (queue *q, order in) {

if (!q->empty) {
  if (q->head > 0) {
    q->head--;
    q->item[q->head] = in;
    
  }
  else {
    int i;  
    for (i = q->tail; i > q->head; i--)
      q->item[i] = q->item[i - 1];
    q->item[0] = in;
    q->tail++;
    if (q->tail == QUEUESIZE)
      q->tail = 0;
  }
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;
    
  }else queueAdd (q, in); 
  return; 
}



// ****************************************************************
void queueAddSort (queue *q, order ord) {
  long i, index;
  int flag;
  
  if(!q->empty) {
    if ((ord.type == 'L') && (ord.action == 'B')) {
      if(ord.price1 >= q->item[q->head].price1)
        queuePush (q, ord);
      else if ((ord.price1 <= q->item[q->tail - 1].price1) && (q->tail > 0))
        queueAdd (q, ord);
      else {
        if(q->tail > q->head) {
          flag = 1;
          i=q->head;
          while ((i < q->tail) && (flag)) {
            if (ord.price1 > q->item[i].price1) {
              index = i;
              flag = 0;
            }
            i++;  
          }
          for (i= q->tail; i > index; i--)
            q->item[i] = q->item[i - 1];
          q->item[index] = ord;
          q->tail++;  
          if (q->tail == QUEUESIZE)
            q->tail = 0;
          if (q->tail == q->head)
            q->full = 1;
          q->empty = 0;
        }
        else {
          flag = 1;
          i = q->head;
          while ((i < QUEUESIZE) && (flag)) {
            if (ord.price1 > q->item[i].price1) {
              index = i;
              flag = 0;
            }
            i++;  
          }
          if (flag) {
            i = 0;
            while((i < q->tail) && (flag)) {
              if (ord.price1 > q->item[i].price1) {
                index = i;
                flag = 0;
              }
              i++;
            }
          }
          if (flag) 
            queueAdd (q, ord);
          else {
            if (q->head < index) {
              for (i = q->head - 1; i < index - 1; i++)
                q->item[i] = q->item[i + 1];
              q->item[index - 1] = ord;
              q->head--;  
              if (q->tail == q->head)
                q->full = 1;
              q->empty = 0;
            }
            else {
              for(i = q->tail; i > index; i--)
                q->item[i] = q->item[i - 1];
              q->item[index] = ord;
              q->tail++;
              if (q->tail == QUEUESIZE)
                q->tail = 0;
              if (q->tail == q->head)
                q->full = 1;
              q->empty = 0; 
            }
          } 
        }
      }
    }
    else if ((ord.type == 'L') && (ord.action == 'S')){
      if (ord.price1 <= q->item[q->head].price1)
        queuePush (q, ord);
      else if ((ord.price1 >= q->item[q->tail - 1].price1) && (q->tail > 0))
        queueAdd (q, ord);
      else {
        if (q->tail > q->head) {
          flag = 1;
          i = q->head;
          while ((i < q->tail) && (flag)) {
            if (ord.price1 < q->item[i].price1) {
              index = i;
              flag = 0;
            }
            i++;
          }
          for (i = q->tail; i > index; i--)
            q->item[i] = q->item[i - 1];
          q->item[index] = ord;
          q->tail++;  
          if (q->tail == QUEUESIZE)
            q->tail = 0;
          if (q->tail == q->head)
            q->full = 1;
          q->empty = 0;
        }
        else {
          flag = 1;
          i = q->head;
          while ((i < QUEUESIZE) && (flag)) {
            if (ord.price1 < q->item[i].price1) {
              index = i;
              flag = 0;
            }
            i++;  
          }
          if (flag) {
            i = 0;
            while ((i < q->tail) && (flag)) {
              if (ord.price1 < q->item[i].price1) {
                index = i;
                flag = 0;
              }
              i++;
            }
          }
          if (flag)
            queueAdd (q, ord);
          else {
            if (q->head < index) {
              for(i = q->head - 1; i < index - 1; i++)
                q->item[i] = q->item[i + 1];
              q->item[index - 1] = ord;
              q->head--;  
              if (q->tail == q->head)
                q->full = 1;
              q->empty = 0;
            }
            else {
              for (i = q->tail; i > index; i--)
                q->item[i] = q->item[i - 1];
              q->item[index] = ord;
              q->tail++;
              if (q->tail == QUEUESIZE)
                q->tail = 0;
              if (q->tail == q->head)
                q->full = 1;
              q->empty = 0; 
            }
          }
        }
      }
    }
  }
  else queueAdd (q, ord);
  return; 
}



// ****************************************************************
void queueDel (queue *q, order *out) {
  *out = q->item[q->head];
  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}



// ****************************************************************
void queueDelete (queue *q) {
  pthread_mutex_destroy (q->mut);
  free (q->mut);  
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}



// ****************************************************************
void queueDelIndex (queue *q, order ord) {
  long i;
  long mid = - 1;
  if (q->tail > q->head) {

    for (i = q->head; i < q->tail; i++) {
      if (ord.oldid == q->item[i].id) {
        mid = i;
        break;
        }
      }
    if (mid == - 1) return;
    for(i = mid; i > q->head; i--)
        q->item[i] = q->item[i - 1];
    q->head++;
    if (q->head == QUEUESIZE)
      q->head = 0;
    if (q->head == q->tail)
      q->empty = 1;
    q->full = 0;
    return;   
    }
    else if ((q->tail == 0) && (!q->empty)) {
      for (i = q->head; i < QUEUESIZE; i++) {
        if (ord.oldid == q->item[i].id) {
          mid = i;
          break;
        }
      }
    if (mid == - 1) return;
    for (i = mid; i > q->head; i--)
        q->item[i] = q->item[i - 1];
        
    q->head++;
    if (q->head == QUEUESIZE)
      q->head = 0;
    if (q->head == q->tail)
      q->empty = 1;
    q->full = 0;
    return;
  }
  else if (!q->empty) {
    for (i = q->head; i < QUEUESIZE; i++) {
      if (ord.oldid == q->item[i].id) {
        mid = i;
        break;
      }
    }
    if (mid == -1) {
      for (i = 0; i < q->tail; i++) {
        if (ord.oldid == q->item[i].id) {
          mid = i;
          break;
        }
      }
    }
    if (mid == - 1) return; 
    if (mid >= q->head) {
      for (i = mid; i > q->head; i--)
        q->item[i]=q->item[i - 1];
      q->head++;
      if (q->head == QUEUESIZE)
        q->head = 0;
      if (q->head == q->tail)
        q->empty = 1;
      q->full = 0;
      return;   
      }
    else if (mid < q->head) {
      for (i = mid; i < q->tail - 1; i++)
        q->item[i] = q->item[i + 1];
        
      q->tail--;  
      q->full = 0;
      return;
    }
  }   
  return;
}



// ****************************************************************
void orderAdd(int flag, order ord) {
  switch (flag) {

    case 0:
      pthread_mutex_lock (buyMarketOrder->mut);
      while (buyMarketOrder->full)
        pthread_cond_wait(buyMarketOrder->notFull, buyMarketOrder->mut);
      queueAdd (buyMarketOrder, ord);
      pthread_mutex_unlock (buyMarketOrder->mut);
      pthread_cond_broadcast (buyMarketOrder->notEmpty);
      break;

    case 1:
      pthread_mutex_lock (sellMarketOrder->mut);
      while (sellMarketOrder->full)
        pthread_cond_wait(sellMarketOrder->notFull, sellMarketOrder->mut);
      queueAdd (sellMarketOrder, ord);
      pthread_mutex_unlock (sellMarketOrder->mut);
      pthread_cond_broadcast (sellMarketOrder->notEmpty);
      break;

    case 2:
      pthread_mutex_lock (buyLimitOrder->mut);
      while (buyLimitOrder->full)
        pthread_cond_wait(buyLimitOrder->notFull, buyLimitOrder->mut);
      queueAddSort (buyLimitOrder, ord);
      pthread_mutex_unlock (buyLimitOrder->mut);
      pthread_cond_broadcast (buyLimitOrder->notEmpty);
      break;

    case 3:
      pthread_mutex_lock (sellLimitOrder->mut);
      while (sellLimitOrder->full)
        pthread_cond_wait(sellLimitOrder->notFull, sellLimitOrder->mut);
      queueAddSort (sellLimitOrder, ord);
      pthread_mutex_unlock (sellLimitOrder->mut);
      pthread_cond_broadcast (sellLimitOrder->notEmpty);
      break;


    default:
      pthread_mutex_lock (cancelOrder->mut);
      while (cancelOrder->full)
        pthread_cond_wait(cancelOrder->notFull, cancelOrder->mut);
      queueAdd (cancelOrder, ord);
      pthread_mutex_unlock (cancelOrder->mut);
      pthread_cond_broadcast (cancelOrder->notEmpty);
      break;


  }
}



// ****************************************************************
void orderPush (int flag, order ord) {

  switch (flag) {

    case 0:
      pthread_mutex_lock (buyMarketOrder->mut);
      while (buyMarketOrder->full) {
        fflush(stdout);
        pthread_cond_wait(buyMarketOrder->notFull, buyMarketOrder->mut);
      }
      queuePush (buyMarketOrder, ord);
      pthread_mutex_unlock (buyMarketOrder->mut);
      pthread_cond_broadcast (buyMarketOrder->notEmpty);
      break;

    case 1:
      pthread_mutex_lock (sellMarketOrder->mut);
      while (sellMarketOrder->full) {
        fflush(stdout);
        pthread_cond_wait(sellMarketOrder->notFull, sellMarketOrder->mut);
      }
      queuePush (sellMarketOrder, ord);
      pthread_mutex_unlock (sellMarketOrder->mut);
      pthread_cond_broadcast (sellMarketOrder->notEmpty);
      break;

    case 2:
      pthread_mutex_lock (buyLimitOrder->mut);
      while (buyLimitOrder->full) {
        fflush(stdout);
        pthread_cond_wait(buyLimitOrder->notFull, buyLimitOrder->mut);
      }
      queuePush (buyLimitOrder, ord);
      pthread_mutex_unlock (buyLimitOrder->mut);
      pthread_cond_broadcast (buyLimitOrder->notEmpty);
      break;

    case 3:
      pthread_mutex_lock (sellLimitOrder->mut);
      while (sellLimitOrder->full) {
        fflush(stdout);
        pthread_cond_wait(sellLimitOrder->notFull, sellLimitOrder->mut);
      }
      queuePush (sellLimitOrder, ord);
      pthread_mutex_unlock (sellLimitOrder->mut);
      pthread_cond_broadcast (sellLimitOrder->notEmpty);
      break;


    default :
      pthread_mutex_lock (cancelOrder->mut);
      while (cancelOrder->full) {
        fflush(stdout);
        pthread_cond_wait(cancelOrder->notFull, cancelOrder->mut);
      }
      queuePush (cancelOrder, ord);
      pthread_mutex_unlock (cancelOrder->mut);
      pthread_cond_broadcast (cancelOrder->notEmpty);
      break;

  }
}



// ****************************************************************
order orderDel (int flag) {
  order ord;
  switch (flag) {

    case 0:
      pthread_mutex_lock (buyMarketOrder->mut);
      while (buyMarketOrder->empty) 
        pthread_cond_wait (buyMarketOrder->notEmpty, buyMarketOrder->mut);
      queueDel (buyMarketOrder, &ord);
      pthread_mutex_unlock (buyMarketOrder->mut);
      pthread_cond_signal (buyMarketOrder->notFull);
      break;

    case 1:
      pthread_mutex_lock (sellMarketOrder->mut);
      while (sellMarketOrder->empty)
        pthread_cond_wait (sellMarketOrder->notEmpty, sellMarketOrder->mut);
      queueDel (sellMarketOrder, &ord);
      pthread_mutex_unlock (sellMarketOrder->mut);
      pthread_cond_signal (sellMarketOrder->notFull);
      break;

    case 2:
      pthread_mutex_lock (buyLimitOrder->mut);
      while (buyLimitOrder->empty)
        pthread_cond_wait (buyLimitOrder->notEmpty, buyLimitOrder->mut);
      queueDel (buyLimitOrder, &ord);
      pthread_mutex_unlock (buyLimitOrder->mut);
      pthread_cond_signal (buyLimitOrder->notFull);
      break;

    case 3:
      pthread_mutex_lock (sellLimitOrder->mut);
      while (sellLimitOrder->empty)
        pthread_cond_wait (sellLimitOrder->notEmpty, sellLimitOrder->mut);
      queueDel (sellLimitOrder, &ord);
      pthread_mutex_unlock (sellLimitOrder->mut);
      pthread_cond_signal (sellLimitOrder->notFull);
      break;


    default :
      pthread_mutex_lock (cancelOrder->mut);
      while (cancelOrder->empty)
        pthread_cond_wait (cancelOrder->notEmpty, cancelOrder->mut);
      queueDel (cancelOrder, &ord);
      pthread_mutex_unlock (cancelOrder->mut);
      pthread_cond_signal (cancelOrder->notFull);
      break;

  }

  return ord;
}



// ****************************************************************
void orderDelIndex (int flag, order ord) {
    switch (flag) {

    case 0:
      pthread_mutex_lock (buyMarketOrder->mut);
      while (buyMarketOrder->empty)
        pthread_cond_wait(buyMarketOrder->notEmpty, buyMarketOrder->mut);
      queueDelIndex (buyMarketOrder, ord);
      pthread_mutex_unlock (buyMarketOrder->mut);
      pthread_cond_signal (buyMarketOrder->notFull);
      break;

    case 1:
      pthread_mutex_lock (sellMarketOrder->mut);
      while (sellMarketOrder->empty)
        pthread_cond_wait(sellMarketOrder->notEmpty, sellMarketOrder->mut);
      queueDelIndex (sellMarketOrder, ord);
      pthread_mutex_unlock (sellMarketOrder->mut);
      pthread_cond_signal (sellMarketOrder->notFull);
      break;

    case 2:
      pthread_mutex_lock (buyLimitOrder->mut);
      while (buyLimitOrder->empty)
        pthread_cond_wait(buyLimitOrder->notEmpty, buyLimitOrder->mut);
      queueDelIndex (buyLimitOrder, ord);
      pthread_mutex_unlock (buyLimitOrder->mut);
      pthread_cond_signal (buyLimitOrder->notFull);
      break;

    case 3:
      pthread_mutex_lock (sellLimitOrder->mut);
      while (sellLimitOrder->empty)
        pthread_cond_wait(sellLimitOrder->notEmpty, sellLimitOrder->mut);
      queueDelIndex (sellLimitOrder, ord);
      pthread_mutex_unlock (sellLimitOrder->mut);
      pthread_cond_signal (sellLimitOrder->notFull);
      break;

    default : break;

    return;
  }
}



// ****************************************************************
transaction *transactionInit() {
  transaction *trans;
  
  trans = (transaction *) malloc (sizeof (transaction));

  trans->marketBuyer = 0;
  trans->marketSeller = 0;
  trans->limitBuyer = 0;
  trans->limitSeller = 0;

  trans->mutBuyer = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (trans->mutBuyer, NULL);
  trans->mutSeller = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (trans->mutSeller, NULL);

  trans->conBuyer = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (trans->conBuyer, NULL);
  trans->conSeller = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (trans->conSeller, NULL);
  trans->buyMarketTransaction = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (trans->buyMarketTransaction, NULL);
  trans->sellMarketTransaction = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (trans->sellMarketTransaction, NULL);
  trans->buyLimitTransaction = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (trans->buyLimitTransaction, NULL);
  trans->sellLimitTransaction = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (trans->sellLimitTransaction, NULL);
  
  return (trans);
}

// ****************************************************************
void makeTransaction (order order1, order order2, int id1, int id2) {
  int volume = order1.vol;
  //static flag = 0;
  if (order1.vol > order2.vol) {
    order1.vol = order1.vol - order2.vol;
    volume = order2.vol;
    if (id1 < 2)
      orderPush (id1, order1);
    else
      orderAdd (id1, order1);
  }
  else if (order1.vol < order2.vol) {
    order2.vol = order2.vol - order1.vol;
    if (id2 < 2)
      orderPush (id2, order2);
    else
      orderAdd(id2, order2);
  }
  //if (!flag) {
    //infile = fopen("log4marketSim.txt", "w");
    //flag = 1;
  //}
  //fprintf(infile, "%ld\t%d\t%d\n", getTimestamp(), currentPriceX10, volume);
  //fflush(infile);

  printf("Timestamp: %ld Price: %d\tVolume: %d\torderid1: %ld orderid2: %ld\n", getTimestamp(), currentPriceX10, volume, order1.id, order2.id);
  return;
}



//**********************************************************
void *Market() {
  
  int i;
  
  while(1) {
    
    order offerB1;
    order offerB2;
    order offerS1;
    order offerS2;

    pthread_mutex_lock (t->mutBuyer);
    
    while ((!t->marketBuyer) && (!t->limitBuyer))
      pthread_cond_wait (t->conBuyer, t->mutBuyer);
    pthread_mutex_unlock (t->mutBuyer);  
    
    pthread_mutex_lock (t->mutSeller);
    
    while ((!t->marketSeller) && (!t->limitSeller))
      pthread_cond_wait (t->conSeller, t->mutSeller);
    pthread_mutex_unlock (t->mutSeller); 
    
    if (t->marketBuyer)
      offerB1 = t->buyMarketOrder;
    if (t->marketSeller)
      offerS1 = t->sellMarketOrder;
    if (t->limitBuyer)
      offerB2 = t->buyLimitOrder;
    if (t->limitSeller)
      offerS2 = t->sellLimitOrder;
      
    
      if ((t->limitBuyer) && (t->limitSeller)) {
        currentPriceX10 = (int)((offerB2.price1 + offerS2.price1) / 2);
        makeTransaction (offerB2, offerS2, 2, 3);
        t->limitBuyer = 0;
        t->limitSeller = 0;
        pthread_cond_signal (t->buyLimitTransaction);
        pthread_cond_signal (t->sellLimitTransaction);
      }
      
      if ((t->marketBuyer) && (t->limitSeller)) {
        currentPriceX10 = offerS2.price1;
        makeTransaction (offerB1, offerS2, 0, 3);
        t->marketBuyer = 0;
        t->limitSeller = 0;
        pthread_cond_signal (t->buyMarketTransaction);
        pthread_cond_signal (t->sellLimitTransaction);
      }

      if ((t->marketSeller) && (t->limitBuyer)) {
        currentPriceX10 = offerB2.price1;
        makeTransaction (offerS1, offerB2, 1, 2);
        t->marketSeller = 0;
        t->limitBuyer = 0;
        pthread_cond_signal (t->sellMarketTransaction);
        pthread_cond_signal (t->buyLimitTransaction);
      }
      
      if ((t->marketBuyer) && (t->marketSeller)) {
        makeTransaction (offerB1, offerS1, 0, 1);
        t->marketBuyer = 0;
        t->marketSeller = 0;
        pthread_cond_signal (t->buyMarketTransaction);
        pthread_cond_signal (t->sellMarketTransaction);
      }     
    
    pthread_cond_signal (buyLimitOrder->notEmpty);
    pthread_cond_signal (sellLimitOrder->notEmpty);

  }
}



// ****************************************************************
void *MarketBuy() {
  order ord;
  pthread_mutex_t mutex;
  pthread_mutex_init (&mutex, NULL);
  
  while(1) {
    ord = orderDel(0);
    
    pthread_mutex_lock (t->mutBuyer);
    t->marketBuyer = 1;
    t->buyMarketOrder = ord;
    pthread_mutex_unlock (t->mutBuyer);
    pthread_cond_signal (t->conBuyer);
    
    pthread_mutex_lock (&mutex);
    while (t->marketBuyer)
      pthread_cond_wait (t->buyMarketTransaction, &mutex); 
    pthread_mutex_unlock (&mutex);
    
    } 
}



//**********************************************************
void *MarketSell() {
  order ord;
  pthread_mutex_t mutex;
  pthread_mutex_init (&mutex, NULL);
  
  while(1) { 
    ord = orderDel(1);
    
    pthread_mutex_lock (t->mutSeller);
    t->marketSeller = 1;
    t->sellMarketOrder = ord;
    pthread_mutex_unlock (t->mutSeller);
    pthread_cond_signal (t->conSeller);
    
    pthread_mutex_lock(&mutex);
    while (t->marketSeller)
      pthread_cond_wait (t->sellMarketTransaction, &mutex);
    pthread_mutex_unlock (&mutex);
    
    }
}



//**********************************************************
void *LimitBuy() {
  order ord;
  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, NULL);
  
  while(1) {
    pthread_mutex_lock (buyLimitOrder->mut);  
    while ((buyLimitOrder->empty) || (buyLimitOrder->item[buyLimitOrder->head].price1 < currentPriceX10))
      pthread_cond_wait (buyLimitOrder->notEmpty, buyLimitOrder->mut);
    
    queueDel (buyLimitOrder, &ord);
    
    
    pthread_mutex_unlock (buyLimitOrder->mut);
    pthread_cond_signal (buyLimitOrder->notFull);
    pthread_mutex_lock (t->mutBuyer);
    t->limitBuyer = 1;
    t->buyLimitOrder = ord;
    pthread_mutex_unlock (t->mutBuyer);
    pthread_cond_signal (t->conBuyer);
  
    pthread_mutex_lock (&mutex);
    
    while (t->limitBuyer)
      pthread_cond_wait (t->buyLimitTransaction, &mutex);
    
    pthread_mutex_unlock (&mutex);
  }
}



//**********************************************************
void *LimitSell() {
  order ord;
  pthread_mutex_t mutex;
  pthread_mutex_init (&mutex, NULL);
  
  while(1) {
    pthread_mutex_lock (sellLimitOrder->mut);
    while ((sellLimitOrder->empty) || (sellLimitOrder->item[sellLimitOrder->head].price1 > currentPriceX10)) {
      pthread_cond_wait (sellLimitOrder->notEmpty, sellLimitOrder->mut);
    }
    queueDel (sellLimitOrder, &ord);
      
    pthread_mutex_unlock (sellLimitOrder->mut);  
    pthread_cond_signal (sellLimitOrder->notFull);
    pthread_mutex_lock (t->mutSeller);
    t->limitSeller = 1;
    t->sellLimitOrder = ord;
    pthread_mutex_unlock (t->mutSeller);
    pthread_cond_signal (t->conSeller);
    
    pthread_mutex_lock (&mutex);
    
    while (t->limitSeller) {
      pthread_cond_wait (t->sellLimitTransaction, &mutex);
    }
    pthread_mutex_unlock (&mutex);
  } 
}



//**********************************************************
void *Cancel() {
  int i;
  order ord;
  while(1) {
    start:
    ord = orderDel(4);
    if ((!buyMarketOrder->empty) && (buyMarketOrder->tail > buyMarketOrder->head)) {
      for (i = buyMarketOrder->head; i < buyMarketOrder->tail; i++) {
        if (ord.oldid == buyMarketOrder->item[i].id) {
          orderDelIndex (0, ord);  
          printf("%ld Buy Market Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    else if ((!buyMarketOrder->empty) && (buyMarketOrder->tail == 0)) {
      for (i = buyMarketOrder->head; i < QUEUESIZE; i++) {
        if (ord.oldid == buyMarketOrder->item[i].id) {
          orderDelIndex (0, ord);
          printf("%ld Buy Market Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    else if (!buyMarketOrder->empty)
    {
      for (i = buyMarketOrder->head; i < QUEUESIZE; i++) {
        if(ord.oldid == buyMarketOrder->item[i].id) {
          orderDelIndex (0, ord);
          printf("%ld Buy Market Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
      for (i = 0; i < buyMarketOrder->tail; i++) {
        if (ord.oldid == buyMarketOrder->item[i].id){
          orderDelIndex (0, ord);
          printf("%ld Buy Market Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    } 

    if ((!sellMarketOrder->empty) && (sellMarketOrder->tail > sellMarketOrder->head)) {
      for (i = sellMarketOrder->head; i < sellMarketOrder->tail; i++) {
        if (ord.oldid == sellMarketOrder->item[i].id) {
          orderDelIndex (1, ord);  
          printf("%ld Sell Market Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    else if ((!sellMarketOrder->empty) && (sellMarketOrder->tail == 0)) {
      for (i = sellMarketOrder->head; i < QUEUESIZE; i++) {
        if (ord.oldid == sellMarketOrder->item[i].id) {
          orderDelIndex (1, ord);
          printf("%ld Sell Market Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    else if (!sellMarketOrder->empty)
    {
      for (i = sellMarketOrder->head; i < QUEUESIZE; i++) {
        if(ord.oldid == sellMarketOrder->item[i].id) {
          orderDelIndex (1, ord);
          printf("%ld Sell Market Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
      for (i = 0; i < sellMarketOrder->tail; i++) {
        if (ord.oldid == sellMarketOrder->item[i].id){
          orderDelIndex (1, ord);
          printf("%ld Sell Market Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    if ((!buyLimitOrder->empty) && (buyLimitOrder->tail > buyLimitOrder->head)) {
      for (i = buyLimitOrder->head; i < buyLimitOrder->tail; i++) {
        if (ord.oldid == buyLimitOrder->item[i].id) {
          orderDelIndex (2, ord);  
          printf("%ld Buy Limit Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    else if ((!buyLimitOrder->empty) && (buyLimitOrder->tail == 0)) {
      for (i = buyLimitOrder->head; i < QUEUESIZE; i++) {
        if (ord.oldid == buyLimitOrder->item[i].id) {
          orderDelIndex (2, ord);
          printf("%ld Buy Limit Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    else if (!buyLimitOrder->empty)
    {
      for (i = buyLimitOrder->head; i < QUEUESIZE; i++) {
        if(ord.oldid == buyLimitOrder->item[i].id) {
          orderDelIndex (2, ord);
          printf("%ld Buy Limit Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
      for (i = 0; i < buyLimitOrder->tail; i++) {
        if (ord.oldid == buyLimitOrder->item[i].id){
          orderDelIndex (2, ord);
          printf("%ld Buy Limit Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    if ((!sellLimitOrder->empty) && (sellLimitOrder->tail > sellLimitOrder->head)) {
      for (i = sellLimitOrder->head; i < sellLimitOrder->tail; i++) {
        if (ord.oldid == sellLimitOrder->item[i].id) {
          orderDelIndex (3, ord);  
          printf("%ld Sell Limit Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    else if ((!sellLimitOrder->empty) && (sellLimitOrder->tail == 0)) {
      for (i = sellLimitOrder->head; i < QUEUESIZE; i++) {
        if (ord.oldid == sellLimitOrder->item[i].id) {
          orderDelIndex (3, ord);
          printf("%ld Sell Limit Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
    else if (!sellLimitOrder->empty)
    {
      for (i = sellLimitOrder->head; i < QUEUESIZE; i++) {
        if(ord.oldid == sellLimitOrder->item[i].id) {
          orderDelIndex (3, ord);
          printf("%ld Sell Limit Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
      for (i = 0; i < sellLimitOrder->tail; i++) {
        if (ord.oldid == sellLimitOrder->item[i].id){
          orderDelIndex (3, ord);
          printf("%ld Sell Limit Order ---> Cancelled\n", ord.oldid);
          goto start;
        }
      }
    }
  }
}



//**********************************************************
