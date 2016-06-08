/*
   Error-checking support functions
   AUP2, Sec. 1.04.2

   Copyright 2003 by Marc J. Rochkind. All rights reserved.
   May be copied only for purposes and under conditions described
   on the Web page www.basepath.com/aup/copyright.htm.

   The Example Files are provided "as is," without any warranty;
   without even the implied warranty of merchantability or fitness
   for a particular purpose. The author and his publisher are not
   responsible for any damages, direct or incidental, resulting
   from the use or non-use of these Example Files.

   The Example Files may contain defects, and some contain deliberate
   coding mistakes that were included for educational reasons.
   You are responsible for determining if and how the Example Files
   are to be used.

*/
#include "defs.h"
#include <pthread.h>

static void ec_mutex(bool lock)
{
   static pthread_mutex_t ec_mtx = PTHREAD_MUTEX_INITIALIZER;
   int errnum;
   char *msg;

   if (lock) {
       if ((errnum = pthread_mutex_lock(&ec_mtx)) == 0)
           return;
   }
   else {
       if ((errnum = pthread_mutex_unlock(&ec_mtx)) == 0)
           return;
   }
   if ((msg = strerror(errnum)) == NULL)
       fprintf(stderr, "Mutex error in ec_* function: %d\n", errnum);
   else
       fprintf(stderr, "Mutex error in ec_* function: %s\n", msg);
   exit(EXIT_FAILURE);
}

/*[ec_atexit_fcn]*/
static void ec_atexit_fcn(void)
{
   ec_print();
}
/*[ec_node]*/
static struct ec_node {
   struct ec_node *ec_next;
   int ec_errno;
   EC_ERRTYPE ec_type;
   char *ec_context;
} *ec_head, ec_node_emergency;
static char ec_s_emergency[100];

const bool ec_in_cleanup = false;
/*[ec_push]*/
#define SEP1 " ["
#define SEP2 ":"
#define SEP3 "] "

void ec_push(const char *fcn, const char *file, int line,
 const char *str, int errno_arg, EC_ERRTYPE type)
{
   struct ec_node node, *p;
   size_t len;
   static bool attexit_called = false;

   ec_mutex(true);
   node.ec_errno = errno_arg;
   node.ec_type = type;
   if (str == NULL)
       str = "";
   len = strlen(fcn) + strlen(SEP1) + strlen(file) + strlen(SEP2) +
     6 + strlen(SEP3) + strlen(str) + 1;
   node.ec_context = (char *)calloc(1, len);
   if (node.ec_context == NULL) {
       if (ec_s_emergency[0] == '\0')
           node.ec_context = ec_s_emergency;
       else
           node.ec_context = "?";
       len = sizeof(ec_s_emergency);
   }
   if (node.ec_context != NULL)
       snprintf(node.ec_context, len, "%s%s%s%s%d%s%s", fcn, SEP1,
         file, SEP2, line, SEP3, str);
   p = (struct ec_node *)calloc(1, sizeof(struct ec_node));
   if (p == NULL && ec_node_emergency.ec_context == NULL)
       p = &ec_node_emergency; /* use just once */
   if (p != NULL) {
       node.ec_next = ec_head;
       ec_head = p;
       *ec_head = node;
   }
   if (!attexit_called) {
       attexit_called = true;
00100         ec_mutex(false);
00101         if (atexit(ec_atexit_fcn) != 0) {
00102             ec_push(fcn, file, line, "atexit failed", errno, EC_ERRNO);
00103             ec_print(); /* so at least the error gets shown */
00104         }
00105     }
00106     else
00107         ec_mutex(false);
00108 }
00109 /*[ec_print]*/
00110 void ec_print(void)
00111 {
00112     struct ec_node *e;
00113     int level = 0;
00114 
00115     ec_mutex(true);
00116     for (e = ec_head; e != NULL; e = e->ec_next, level++) {
00117         char buf[200], buf2[25 + sizeof(buf)];
00118 
00119         if (e == &ec_node_emergency)
00120             fprintf(stderr, "\t*** Trace may be incomplete ***\n");
00121         syserrmsgtype(buf, sizeof(buf), e->ec_context,
00122           e->ec_next == NULL ? e->ec_errno : 0, e->ec_type);
00123         snprintf(buf2, sizeof(buf2), "%s\t%d: %s",
00124           (level == 0? "ERROR:" : ""), level, buf);
00125         fprintf(stderr, "%s\n", buf2);
00126         logfmt(buf2);
00127     }
00128     ec_mutex(false);
00129 }
00130 /*[ec_reinit]*/
00131 void ec_reinit(void)
00132 {
00133     struct ec_node *e, *e_next;
00134 
00135     ec_mutex(true);
00136     for (e = ec_head; e != NULL; e = e_next) {
00137         e_next = e->ec_next;
00138         if (e->ec_context != ec_s_emergency)
00139             free(e->ec_context);
00140         if (e != &ec_node_emergency)
00141             free(e);
00142     }
00143     ec_head = NULL;
00144     memset(&ec_node_emergency, 0, sizeof(ec_node_emergency));
00145     memset(&ec_s_emergency, 0, sizeof(ec_s_emergency));
00146     ec_mutex(false);
00147 }
00148 /*[ec_warn]*/
00149 void ec_warn(void)
00150 {
00151     fprintf(stderr, "***WARNING: Control flowed into EC_CLEANUP_BGN\n");
00152 }
00153 /*[]*/
