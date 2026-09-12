#include "common.h"
/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         socks_yyparse
#define yylex           socks_yylex
#define yyerror         socks_yyerror
#define yydebug         socks_yydebug
#define yynerrs         socks_yynerrs
#define yylval          socks_yylval
#define yychar          socks_yychar

/* First part of user prologue.  */
#line 46 "config_parse.y"


#include "yacconfig.h"

#if !SOCKS_CLIENT

#include "monitor.h"

#endif /* !SOCKS_CLIENT */

static const char rcsid[] =
"$Id: config_parse.y,v 1.703.4.8.2.8.4.14.4.2 2024/11/21 10:22:42 michaels Exp $";

#if HAVE_LIBWRAP && (!SOCKS_CLIENT)
   extern jmp_buf tcpd_buf;
#endif /* HAVE_LIBWRAP && (!SOCKS_CLIENT) */

extern void yyrestart(FILE *fp);

typedef enum { from, to, bounce } addresscontext_t;

static int
ipaddr_requires_netmask(const addresscontext_t context,
                        const objecttype_t objecttype);
/*
 * Returns true if an ipaddress used in the context of "objecttype" requires
 * a netmask, or false otherwise.
 *
 * "isfrom" is true if the address is to be used in the source/from
 * context, and false otherwise.
 */

static void
addnumber(size_t *numberc, long long *numberv[], const long long number);

static void
addrinit(ruleaddr_t *addr, const int netmask_required);

static void
gwaddrinit(sockshost_t *addr);

static void
routeinit(route_t *route);

#if SOCKS_CLIENT
static void parseclientenv(int *haveproxyserver);
/*
 * parses client environment, if any.
 * If a proxy server is configured in environment, "haveproxyserver" is set
 * to true upon return.  If not, it is set to false.
 */

static char *serverstring2gwstring(const char *server, const int version,
                                   char *gw, const size_t gwsize);
/*
 * Converts a gateway specified in environment to the format expected
 * in a socks.conf file.
 * "server" is the address specified in the environment,
 * "version" the kind of server address,
 * "gw", of size "gwsize", is the string to store the converted address in.
 *
 * Returns "gw" on success, exits on error.
 */

#define alarminit()
#define SET_TCPOPTION(logobject, level, attr)

#else /* !SOCKS_CLIENT */

/*
 * Reset pointers to point away from object-specific memory to global
 * memory.  Should be called after adding the object.
 */
static void post_addrule(void);

/*
 * Sets up various things after a object has been parsed, but before it has
 * been added.  Should be called before adding the object.
 *
 */
static void pre_addrule(struct rule_t *rule);
static void pre_addmonitor(monitor_t *monitor);

/*
 * Prepare pointers to point to the correct memory for adding a
 * new objects.  Should always be called once we know what type of
 * object we are dealing with.
 */
static void ruleinit(rule_t *rule);
static void monitorinit(monitor_t *monitor);
static void alarminit(void);

static int configure_privileges(void);
/*
 * Sets up privileges/userids.
 */

static int
checkugid(uid_t *uid, gid_t *gid, unsigned char *isset, const char *type);

#define SET_TCPOPTION(tcp, level, attr)                                        \
do {                                                                           \
   (tcp)->isconfigured              = 1;                                       \
                                                                               \
   (tcp)->attr                      = 1;                                       \
   (tcp)->__CONCAT(attr, _loglevel) = cloglevel;                               \
} while (/* CONSTCOND */ 0)

/*
 * Let commandline-options override configfile-options.
 * Currently there's only one such option.
 */
#define LOG_CMDLINE_OVERRIDE(name, newvalue, oldvalue, fmt)                    \
do {                                                                           \
   slog(LOG_NOTICE,                                                            \
        "%s: %s commandline value \"" fmt "\" overrides "                      \
        "config-file value \"" fmt "\" set in file %s",                        \
        function, name, (newvalue), (oldvalue), sockscf.option.configfile);    \
} while (/* CONSTCOND */ 0 )

#define CMDLINE_OVERRIDE(cmdline, option)                                      \
do {                                                                           \
   if ((cmdline)->debug_isset) {                                               \
      if ((option)->debug != (cmdline)->debug)                                 \
         LOG_CMDLINE_OVERRIDE("debug",                                         \
                              (cmdline)->debug,                                \
                              (option)->debug,                                 \
                              "%d");                                           \
                                                                               \
      (option)->debug      = (cmdline)->debug;                                 \
      (option)->debug_isset= (cmdline)->debug_isset;                           \
   }                                                                           \
} while (/* CONSTCOND */ 0)

#endif /* !SOCKS_CLIENT */

extern int  yylineno;
extern char *yytext;
extern char currentlexline[];
extern char previouslexline[];

static const char *function = "configparsing()";

/*
 * Globals because used by functions for reporting parsing errors in
 * parse_util.c
 */
unsigned char   *atype;         /* atype of new address.               */
unsigned char  parsingconfig;   /* currently parsing config?          */

/*
 * for case we are unable to (re-)open logfiles operator specifies.
 */

#if !SOCKS_CLIENT
static logtype_t       old_log,           old_errlog;
static unsigned char   logformat_seen;  /* reset for each config parse.       */
#endif /* !SOCKS_CLIENT */

static int             failed_to_add_log, failed_to_add_errlog;

static unsigned char   add_to_errlog;   /* adding file to errlog or regular?  */

static objecttype_t    objecttype;      /* current object_type we are parsing.*/


#if !SOCKS_CLIENT
static  logspecial_t                *logspecial;
static warn_protocol_tcp_options_t  *tcpoptions;

static interfaceprotocol_t *ifproto;  /* new interfaceprotocol settings.      */

static monitor_t       monitor;       /* new monitor.                         */
static monitor_if_t    *monitorif;    /* new monitor interface.               */
static int             *alarmside;    /* data-side to monitor (read/write).   */

static int             cloglevel;     /* current loglevel.                    */

static rule_t          rule;          /* new rule.                            */

static shmem_object_t  ss;
static int session_isset;
static shmem_object_t  bw;
static int bw_isset;


#endif /* !SOCKS_CLIENT */

static unsigned char   *hostidoption_isset;

static long long       *numberv;
static size_t          numberc;

#if !SOCKS_CLIENT && HAVE_SOCKS_HOSTID
static unsigned char   *hostindex;
#endif /* !SOCKS_CLIENT && HAVE_SOCKS_HOSTID  */

static timeout_t       *timeout = &sockscf.timeout;           /* default.     */

static socketoption_t  socketopt;

static serverstate_t   *state;
static route_t         route;         /* new route.                           */
static sockshost_t     gw;            /* new gateway.                         */

static ruleaddr_t      src;            /* new src.                            */
static ruleaddr_t      dst;            /* new dst.                            */
static ruleaddr_t      hostid;         /* new hostid.                         */
static ruleaddr_t      rdr_from;       /* new redirect from.                  */
static ruleaddr_t      rdr_to;         /* new redirect to.                    */

#if BAREFOOTD
static ruleaddr_t      bounceto;       /* new bounce-to address.              */
#endif /* BAREFOOTD */

static ruleaddr_t      *ruleaddr;      /* current ruleaddr                    */
static extension_t     *extension;     /* new extensions                      */


static struct in_addr  *ipv4;          /* new ip address                      */
static struct in_addr  *netmask_v4;    /* new netmask                         */

static struct in6_addr *ipv6;          /* new ip address                      */
static unsigned int    *netmask_v6;    /* new netmask                         */
static uint32_t        *scopeid_v6;    /* new scopeid.                        */

static struct in_addr  *ipvany;        /* new ip address                      */
static struct in_addr  *netmask_vany;  /* new netmask                         */

static int             netmask_required;/*
                                         * netmask required for this
                                         * address?
                                         */
static char            *domain;        /* new domain.                         */
static char            *ifname;        /* new ifname.                         */
static char            *url;           /* new url.                            */

static in_port_t       *port_tcp;      /* new TCP port number.                */
static in_port_t       *port_udp;      /* new UDP port number.                */

static int             *cmethodv;      /* new client authmethods.             */
static size_t          *cmethodc;      /* number of them.                     */
static int             *smethodv;      /* new socks authmethods.              */
static size_t          *smethodc;      /* number of them.                     */

static enum operator_t *operator;      /* new port operator.                  */

#if HAVE_GSSAPI
static char            *gssapiservicename; /* new gssapiservice.              */
static char            *gssapikeytab;      /* new gssapikeytab.               */
static gssapi_enc_t    *gssapiencryption;  /* new encryption status.          */
#endif /* HAVE_GSSAPI */

#if !SOCKS_CLIENT && HAVE_LDAP
/*
 * new ldapauthorisation server details.  Used for checking if an already
 * (GSSAPI) authenticated user is member of the appropriate LDAP group.
 */
static ldapauthorisation_t    *ldapauthorisation;


/*
 * new ldapauthorisation auth server details.
 * Used for doing LDAP-based authentication of a new client.
 */
static ldapauthentication_t   *ldapauthentication;

#endif /* SOCKS_SERVER && HAVE_LDAP */

#if !SOCKS_CLIENT && HAVE_PAC
static char            *b64;        /* new b64 encoded sid.                   */
#endif /* !SOCKS_CLIENT && HAVE_PAC */

#if DEBUG
#define YYDEBUG 1
#endif /* DEBUG */

#define ADDMETHOD(method, methodc, methodv)                                    \
do {                                                                           \
   if (methodisset((method), (methodv), (methodc)))                            \
      yywarnx("duplicate method: %s.  Already set on this methodline",         \
              method2string((method)));                                        \
   else {                                                                      \
      if ((methodc) >= METHODS_KNOWN) {                                        \
         yyerrorx("too many authmethods (%lu, max is %ld)",                    \
                  (unsigned long)(methodc), (long)METHODS_KNOWN);              \
         SERRX(methodc);                                                       \
      }                                                                        \
                                                                               \
      /*                                                                       \
       * check if we have the external libraries required for the method.      \
       */                                                                      \
      switch (method) {                                                        \
         case AUTHMETHOD_BSDAUTH:                                              \
            if (!HAVE_BSDAUTH)                                                 \
               yyerrorx_nolib("bsdauth");                                      \
            break;                                                             \
                                                                               \
         case AUTHMETHOD_GSSAPI:                                               \
            if (!HAVE_GSSAPI)                                                  \
               yyerrorx_nolib("GSSAPI");                                       \
                                                                               \
            break;                                                             \
                                                                               \
         case AUTHMETHOD_RFC931:                                               \
            if (!HAVE_LIBWRAP)                                                 \
               yyerrorx_nolib("libwrap");                                      \
            break;                                                             \
                                                                               \
         case AUTHMETHOD_PAM_ANY:                                              \
         case AUTHMETHOD_PAM_ADDRESS:                                          \
         case AUTHMETHOD_PAM_USERNAME:                                         \
            if (!HAVE_PAM)                                                     \
               yyerrorx_nolib("PAM");                                          \
            break;                                                             \
                                                                               \
         case AUTHMETHOD_LDAPAUTH:                                             \
            if (!HAVE_LDAP)                                                    \
               yyerrorx_nolib("LDAP");                                         \
            break;                                                             \
      }                                                                        \
                                                                               \
      methodv[(methodc)++] = method;                                           \
   }                                                                           \
} while (0)

#define ASSIGN_NUMBER(number, op, checkagainst, object, issigned)              \
do {                                                                           \
   if (!((number) op (checkagainst)))                                          \
      yyerrorx("number (%lld) must be " #op " %lld (" #checkagainst ")",       \
               (long long)(number), (long long)(checkagainst));                \
                                                                               \
   if (issigned) {                                                             \
      if ((long long)(number) < minvalueoftype(sizeof(object)))                \
         yyerrorx("number %lld is too small.  Minimum is %lld",                \
                  (long long)number, minvalueoftype(sizeof(object)));          \
                                                                               \
      if ((long long)(number) > maxvalueoftype(sizeof(object)))                \
         yyerrorx("number %lld is too large.  Maximum is %lld",                \
                  (long long)number,  maxvalueoftype(sizeof(object)));         \
   }                                                                           \
   else  {                                                                     \
      if ((unsigned long long)(number) < uminvalueoftype(sizeof(object)))      \
         yyerrorx("number %llu is too small.  Minimum is %llu",                \
                  (unsigned long long)number, uminvalueoftype(sizeof(object)));\
                                                                               \
      if ((unsigned long long)(number) > umaxvalueoftype(sizeof(object)))      \
         yyerrorx("number %llu is too large.  Maximum is %llu",                \
                  (unsigned long long)number, umaxvalueoftype(sizeof(object)));\
   }                                                                           \
                                                                               \
   (object) = (number);                                                        \
} while (0)

#define ASSIGN_PORTNUMBER(portnumber, object)                                  \
do {                                                                           \
   /* includes 0 and MAXPORT because the exp might be "> 0" or "< MAXPORT". */ \
   ASSIGN_NUMBER(portnumber, >=,  0,         (object), 0);                     \
   ASSIGN_NUMBER(portnumber, <=, IP_MAXPORT, (object), 0);                     \
                                                                               \
   (object) = htons((in_port_t)(portnumber));                                  \
} while (0)

#define ASSIGN_THROTTLE_SECONDS(number, obj, issigned)     \
            ASSIGN_NUMBER((number), >, 0, obj, issigned)
#define ASSIGN_THROTTLE_CLIENTS(number, obj, issigned)     \
            ASSIGN_NUMBER((number), >, 0, obj, issigned)
#define ASSIGN_MAXSESSIONS(number, obj, issigned)          \
            ASSIGN_NUMBER((number), >, 0, obj, issigned)

#line 449 "config_parse.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_SOCKS_YY_Y_TAB_H_INCLUDED
# define YY_SOCKS_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int socks_yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ALARM = 258,                   /* ALARM  */
    ALARMTYPE_DATA = 259,          /* ALARMTYPE_DATA  */
    ALARMTYPE_DISCONNECT = 260,    /* ALARMTYPE_DISCONNECT  */
    ALARMIF_INTERNAL = 261,        /* ALARMIF_INTERNAL  */
    ALARMIF_EXTERNAL = 262,        /* ALARMIF_EXTERNAL  */
    TCPOPTION_DISABLED = 263,      /* TCPOPTION_DISABLED  */
    ECN = 264,                     /* ECN  */
    SACK = 265,                    /* SACK  */
    TIMESTAMPS = 266,              /* TIMESTAMPS  */
    WSCALE = 267,                  /* WSCALE  */
    MTU_ERROR = 268,               /* MTU_ERROR  */
    CLIENTCOMPATIBILITY = 269,     /* CLIENTCOMPATIBILITY  */
    NECGSSAPI = 270,               /* NECGSSAPI  */
    CLIENTRULE = 271,              /* CLIENTRULE  */
    HOSTIDRULE = 272,              /* HOSTIDRULE  */
    SOCKSRULE = 273,               /* SOCKSRULE  */
    COMPATIBILITY = 274,           /* COMPATIBILITY  */
    SAMEPORT = 275,                /* SAMEPORT  */
    DRAFT_5_05 = 276,              /* DRAFT_5_05  */
    CONNECTTIMEOUT = 277,          /* CONNECTTIMEOUT  */
    TCP_FIN_WAIT = 278,            /* TCP_FIN_WAIT  */
    CPU = 279,                     /* CPU  */
    MASK = 280,                    /* MASK  */
    SCHEDULE = 281,                /* SCHEDULE  */
    CPUMASK_ANYCPU = 282,          /* CPUMASK_ANYCPU  */
    DEBUGGING = 283,               /* DEBUGGING  */
    DEPRECATED = 284,              /* DEPRECATED  */
    ERRORLOG = 285,                /* ERRORLOG  */
    LOGOUTPUT = 286,               /* LOGOUTPUT  */
    LOGFILE = 287,                 /* LOGFILE  */
    LOGTYPE_ERROR = 288,           /* LOGTYPE_ERROR  */
    LOGTYPE_TCP_DISABLED = 289,    /* LOGTYPE_TCP_DISABLED  */
    LOGTYPE_TCP_ENABLED = 290,     /* LOGTYPE_TCP_ENABLED  */
    LOGIF_INTERNAL = 291,          /* LOGIF_INTERNAL  */
    LOGIF_EXTERNAL = 292,          /* LOGIF_EXTERNAL  */
    ERRORVALUE = 293,              /* ERRORVALUE  */
    EXTENSION = 294,               /* EXTENSION  */
    BIND = 295,                    /* BIND  */
    PRIVILEGED = 296,              /* PRIVILEGED  */
    EXTERNAL_PROTOCOL = 297,       /* EXTERNAL_PROTOCOL  */
    INTERNAL_PROTOCOL = 298,       /* INTERNAL_PROTOCOL  */
    EXTERNAL_ROTATION = 299,       /* EXTERNAL_ROTATION  */
    SAMESAME = 300,                /* SAMESAME  */
    GROUPNAME = 301,               /* GROUPNAME  */
    HOSTID = 302,                  /* HOSTID  */
    HOSTINDEX = 303,               /* HOSTINDEX  */
    INTERFACE = 304,               /* INTERFACE  */
    SOCKETOPTION_SYMBOLICVALUE = 305, /* SOCKETOPTION_SYMBOLICVALUE  */
    INTERNAL = 306,                /* INTERNAL  */
    EXTERNAL = 307,                /* EXTERNAL  */
    INTERNALSOCKET = 308,          /* INTERNALSOCKET  */
    EXTERNALSOCKET = 309,          /* EXTERNALSOCKET  */
    IOTIMEOUT = 310,               /* IOTIMEOUT  */
    IOTIMEOUT_TCP = 311,           /* IOTIMEOUT_TCP  */
    IOTIMEOUT_UDP = 312,           /* IOTIMEOUT_UDP  */
    NEGOTIATETIMEOUT = 313,        /* NEGOTIATETIMEOUT  */
    LIBWRAP_FILE = 314,            /* LIBWRAP_FILE  */
    LOGLEVEL = 315,                /* LOGLEVEL  */
    SOCKSMETHOD = 316,             /* SOCKSMETHOD  */
    CLIENTMETHOD = 317,            /* CLIENTMETHOD  */
    METHOD = 318,                  /* METHOD  */
    METHODNAME = 319,              /* METHODNAME  */
    NONE = 320,                    /* NONE  */
    BSDAUTH = 321,                 /* BSDAUTH  */
    GSSAPI = 322,                  /* GSSAPI  */
    PAM_ADDRESS = 323,             /* PAM_ADDRESS  */
    PAM_ANY = 324,                 /* PAM_ANY  */
    PAM_USERNAME = 325,            /* PAM_USERNAME  */
    RFC931 = 326,                  /* RFC931  */
    UNAME = 327,                   /* UNAME  */
    MONITOR = 328,                 /* MONITOR  */
    PROCESSTYPE = 329,             /* PROCESSTYPE  */
    PROC_MAXREQUESTS = 330,        /* PROC_MAXREQUESTS  */
    PROC_MAXLIFETIME = 331,        /* PROC_MAXLIFETIME  */
    REALM = 332,                   /* REALM  */
    REALNAME = 333,                /* REALNAME  */
    RESOLVEPROTOCOL = 334,         /* RESOLVEPROTOCOL  */
    REQUIRED = 335,                /* REQUIRED  */
    SCHEDULEPOLICY = 336,          /* SCHEDULEPOLICY  */
    SERVERCONFIG = 337,            /* SERVERCONFIG  */
    CLIENTCONFIG = 338,            /* CLIENTCONFIG  */
    SOCKET = 339,                  /* SOCKET  */
    CLIENTSIDE_SOCKET = 340,       /* CLIENTSIDE_SOCKET  */
    SNDBUF = 341,                  /* SNDBUF  */
    RCVBUF = 342,                  /* RCVBUF  */
    SOCKETPROTOCOL = 343,          /* SOCKETPROTOCOL  */
    SOCKETOPTION_OPTID = 344,      /* SOCKETOPTION_OPTID  */
    SRCHOST = 345,                 /* SRCHOST  */
    NODNSMISMATCH = 346,           /* NODNSMISMATCH  */
    NODNSUNKNOWN = 347,            /* NODNSUNKNOWN  */
    CHECKREPLYAUTH = 348,          /* CHECKREPLYAUTH  */
    USERNAME = 349,                /* USERNAME  */
    USER_PRIVILEGED = 350,         /* USER_PRIVILEGED  */
    USER_UNPRIVILEGED = 351,       /* USER_UNPRIVILEGED  */
    USER_LIBWRAP = 352,            /* USER_LIBWRAP  */
    WORD__IN = 353,                /* WORD__IN  */
    ROUTE = 354,                   /* ROUTE  */
    VIA = 355,                     /* VIA  */
    GLOBALROUTEOPTION = 356,       /* GLOBALROUTEOPTION  */
    BADROUTE_EXPIRE = 357,         /* BADROUTE_EXPIRE  */
    MAXFAIL = 358,                 /* MAXFAIL  */
    PORT = 359,                    /* PORT  */
    NUMBER = 360,                  /* NUMBER  */
    BANDWIDTH = 361,               /* BANDWIDTH  */
    BOUNCE = 362,                  /* BOUNCE  */
    BSDAUTHSTYLE = 363,            /* BSDAUTHSTYLE  */
    BSDAUTHSTYLENAME = 364,        /* BSDAUTHSTYLENAME  */
    COMMAND = 365,                 /* COMMAND  */
    COMMAND_BIND = 366,            /* COMMAND_BIND  */
    COMMAND_CONNECT = 367,         /* COMMAND_CONNECT  */
    COMMAND_UDPASSOCIATE = 368,    /* COMMAND_UDPASSOCIATE  */
    COMMAND_BINDREPLY = 369,       /* COMMAND_BINDREPLY  */
    COMMAND_UDPREPLY = 370,        /* COMMAND_UDPREPLY  */
    ACTION = 371,                  /* ACTION  */
    FROM = 372,                    /* FROM  */
    TO = 373,                      /* TO  */
    GSSAPIENCTYPE = 374,           /* GSSAPIENCTYPE  */
    GSSAPIENC_ANY = 375,           /* GSSAPIENC_ANY  */
    GSSAPIENC_CLEAR = 376,         /* GSSAPIENC_CLEAR  */
    GSSAPIENC_INTEGRITY = 377,     /* GSSAPIENC_INTEGRITY  */
    GSSAPIENC_CONFIDENTIALITY = 378, /* GSSAPIENC_CONFIDENTIALITY  */
    GSSAPIENC_PERMESSAGE = 379,    /* GSSAPIENC_PERMESSAGE  */
    GSSAPIKEYTAB = 380,            /* GSSAPIKEYTAB  */
    GSSAPISERVICE = 381,           /* GSSAPISERVICE  */
    GSSAPISERVICENAME = 382,       /* GSSAPISERVICENAME  */
    GSSAPIKEYTABNAME = 383,        /* GSSAPIKEYTABNAME  */
    IPV4 = 384,                    /* IPV4  */
    IPV6 = 385,                    /* IPV6  */
    IPVANY = 386,                  /* IPVANY  */
    DOMAINNAME = 387,              /* DOMAINNAME  */
    IFNAME = 388,                  /* IFNAME  */
    URL = 389,                     /* URL  */
    LDAPATTRIBUTE = 390,           /* LDAPATTRIBUTE  */
    LDAPATTRIBUTE_AD = 391,        /* LDAPATTRIBUTE_AD  */
    LDAPATTRIBUTE_HEX = 392,       /* LDAPATTRIBUTE_HEX  */
    LDAPATTRIBUTE_AD_HEX = 393,    /* LDAPATTRIBUTE_AD_HEX  */
    LDAPBASEDN = 394,              /* LDAPBASEDN  */
    LDAP_BASEDN = 395,             /* LDAP_BASEDN  */
    LDAPBASEDN_HEX = 396,          /* LDAPBASEDN_HEX  */
    LDAPBASEDN_HEX_ALL = 397,      /* LDAPBASEDN_HEX_ALL  */
    LDAPCERTFILE = 398,            /* LDAPCERTFILE  */
    LDAPCERTPATH = 399,            /* LDAPCERTPATH  */
    LDAPPORT = 400,                /* LDAPPORT  */
    LDAPPORTSSL = 401,             /* LDAPPORTSSL  */
    LDAPDEBUG = 402,               /* LDAPDEBUG  */
    LDAPDEPTH = 403,               /* LDAPDEPTH  */
    LDAPAUTO = 404,                /* LDAPAUTO  */
    LDAPSEARCHTIME = 405,          /* LDAPSEARCHTIME  */
    LDAPDOMAIN = 406,              /* LDAPDOMAIN  */
    LDAP_DOMAIN = 407,             /* LDAP_DOMAIN  */
    LDAPFILTER = 408,              /* LDAPFILTER  */
    LDAPFILTER_AD = 409,           /* LDAPFILTER_AD  */
    LDAPFILTER_HEX = 410,          /* LDAPFILTER_HEX  */
    LDAPFILTER_AD_HEX = 411,       /* LDAPFILTER_AD_HEX  */
    LDAPGROUP = 412,               /* LDAPGROUP  */
    LDAPGROUP_NAME = 413,          /* LDAPGROUP_NAME  */
    LDAPGROUP_HEX = 414,           /* LDAPGROUP_HEX  */
    LDAPGROUP_HEX_ALL = 415,       /* LDAPGROUP_HEX_ALL  */
    LDAPKEYTAB = 416,              /* LDAPKEYTAB  */
    LDAPKEYTABNAME = 417,          /* LDAPKEYTABNAME  */
    LDAPDEADTIME = 418,            /* LDAPDEADTIME  */
    LDAPSERVER = 419,              /* LDAPSERVER  */
    LDAPSERVER_NAME = 420,         /* LDAPSERVER_NAME  */
    LDAPAUTHSERVER = 421,          /* LDAPAUTHSERVER  */
    LDAPAUTHKEYTAB = 422,          /* LDAPAUTHKEYTAB  */
    LDAPSSL = 423,                 /* LDAPSSL  */
    LDAPCERTCHECK = 424,           /* LDAPCERTCHECK  */
    LDAPKEEPREALM = 425,           /* LDAPKEEPREALM  */
    LDAPTIMEOUT = 426,             /* LDAPTIMEOUT  */
    LDAPCACHE = 427,               /* LDAPCACHE  */
    LDAPCACHEPOS = 428,            /* LDAPCACHEPOS  */
    LDAPCACHENEG = 429,            /* LDAPCACHENEG  */
    LDAPURL = 430,                 /* LDAPURL  */
    LDAP_URL = 431,                /* LDAP_URL  */
    LDAPAUTHBASEDN = 432,          /* LDAPAUTHBASEDN  */
    LDAPAUTHBASEDN_HEX = 433,      /* LDAPAUTHBASEDN_HEX  */
    LDAPAUTHBASEDN_HEX_ALL = 434,  /* LDAPAUTHBASEDN_HEX_ALL  */
    LDAPAUTHURL = 435,             /* LDAPAUTHURL  */
    LDAPAUTHPORT = 436,            /* LDAPAUTHPORT  */
    LDAPAUTHPORTSSL = 437,         /* LDAPAUTHPORTSSL  */
    LDAPAUTHDEBUG = 438,           /* LDAPAUTHDEBUG  */
    LDAPAUTHSSL = 439,             /* LDAPAUTHSSL  */
    LDAPAUTHAUTO = 440,            /* LDAPAUTHAUTO  */
    LDAPAUTHCERTCHECK = 441,       /* LDAPAUTHCERTCHECK  */
    LDAPAUTHFILTER = 442,          /* LDAPAUTHFILTER  */
    LDAPAUTHDOMAIN = 443,          /* LDAPAUTHDOMAIN  */
    LDAPAUTHCERTFILE = 444,        /* LDAPAUTHCERTFILE  */
    LDAPAUTHCERTPATH = 445,        /* LDAPAUTHCERTPATH  */
    LDAPAUTHKEEPREALM = 446,       /* LDAPAUTHKEEPREALM  */
    LDAP_FILTER = 447,             /* LDAP_FILTER  */
    LDAP_ATTRIBUTE = 448,          /* LDAP_ATTRIBUTE  */
    LDAP_CERTFILE = 449,           /* LDAP_CERTFILE  */
    LDAP_CERTPATH = 450,           /* LDAP_CERTPATH  */
    LIBWRAPSTART = 451,            /* LIBWRAPSTART  */
    LIBWRAP_ALLOW = 452,           /* LIBWRAP_ALLOW  */
    LIBWRAP_DENY = 453,            /* LIBWRAP_DENY  */
    LIBWRAP_HOSTS_ACCESS = 454,    /* LIBWRAP_HOSTS_ACCESS  */
    LINE = 455,                    /* LINE  */
    OPERATOR = 456,                /* OPERATOR  */
    PACSID = 457,                  /* PACSID  */
    PACSID_B64 = 458,              /* PACSID_B64  */
    PACSID_FLAG = 459,             /* PACSID_FLAG  */
    PACSID_NAME = 460,             /* PACSID_NAME  */
    PAMSERVICENAME = 461,          /* PAMSERVICENAME  */
    PROTOCOL = 462,                /* PROTOCOL  */
    PROTOCOL_TCP = 463,            /* PROTOCOL_TCP  */
    PROTOCOL_UDP = 464,            /* PROTOCOL_UDP  */
    PROTOCOL_FAKE = 465,           /* PROTOCOL_FAKE  */
    PROXYPROTOCOL = 466,           /* PROXYPROTOCOL  */
    PROXYPROTOCOL_SOCKS_V4 = 467,  /* PROXYPROTOCOL_SOCKS_V4  */
    PROXYPROTOCOL_SOCKS_V5 = 468,  /* PROXYPROTOCOL_SOCKS_V5  */
    PROXYPROTOCOL_HTTP = 469,      /* PROXYPROTOCOL_HTTP  */
    PROXYPROTOCOL_UPNP = 470,      /* PROXYPROTOCOL_UPNP  */
    REDIRECT = 471,                /* REDIRECT  */
    SENDSIDE = 472,                /* SENDSIDE  */
    RECVSIDE = 473,                /* RECVSIDE  */
    SERVICENAME = 474,             /* SERVICENAME  */
    SESSION_INHERITABLE = 475,     /* SESSION_INHERITABLE  */
    SESSIONMAX = 476,              /* SESSIONMAX  */
    SESSIONTHROTTLE = 477,         /* SESSIONTHROTTLE  */
    SESSIONSTATE_KEY = 478,        /* SESSIONSTATE_KEY  */
    SESSIONSTATE_MAX = 479,        /* SESSIONSTATE_MAX  */
    SESSIONSTATE_THROTTLE = 480,   /* SESSIONSTATE_THROTTLE  */
    RULE_LOG = 481,                /* RULE_LOG  */
    RULE_LOG_CONNECT = 482,        /* RULE_LOG_CONNECT  */
    RULE_LOG_DATA = 483,           /* RULE_LOG_DATA  */
    RULE_LOG_DISCONNECT = 484,     /* RULE_LOG_DISCONNECT  */
    RULE_LOG_ERROR = 485,          /* RULE_LOG_ERROR  */
    RULE_LOG_IOOPERATION = 486,    /* RULE_LOG_IOOPERATION  */
    RULE_LOG_TCPINFO = 487,        /* RULE_LOG_TCPINFO  */
    STATEKEY = 488,                /* STATEKEY  */
    UDPPORTRANGE = 489,            /* UDPPORTRANGE  */
    UDPCONNECTDST = 490,           /* UDPCONNECTDST  */
    USER = 491,                    /* USER  */
    GROUP = 492,                   /* GROUP  */
    VERDICT_BLOCK = 493,           /* VERDICT_BLOCK  */
    VERDICT_PASS = 494,            /* VERDICT_PASS  */
    YES = 495,                     /* YES  */
    NO = 496,                      /* NO  */
    LOGFORMAT = 497,               /* LOGFORMAT  */
    LOGFORMAT_VALUE = 498          /* LOGFORMAT_VALUE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define ALARM 258
#define ALARMTYPE_DATA 259
#define ALARMTYPE_DISCONNECT 260
#define ALARMIF_INTERNAL 261
#define ALARMIF_EXTERNAL 262
#define TCPOPTION_DISABLED 263
#define ECN 264
#define SACK 265
#define TIMESTAMPS 266
#define WSCALE 267
#define MTU_ERROR 268
#define CLIENTCOMPATIBILITY 269
#define NECGSSAPI 270
#define CLIENTRULE 271
#define HOSTIDRULE 272
#define SOCKSRULE 273
#define COMPATIBILITY 274
#define SAMEPORT 275
#define DRAFT_5_05 276
#define CONNECTTIMEOUT 277
#define TCP_FIN_WAIT 278
#define CPU 279
#define MASK 280
#define SCHEDULE 281
#define CPUMASK_ANYCPU 282
#define DEBUGGING 283
#define DEPRECATED 284
#define ERRORLOG 285
#define LOGOUTPUT 286
#define LOGFILE 287
#define LOGTYPE_ERROR 288
#define LOGTYPE_TCP_DISABLED 289
#define LOGTYPE_TCP_ENABLED 290
#define LOGIF_INTERNAL 291
#define LOGIF_EXTERNAL 292
#define ERRORVALUE 293
#define EXTENSION 294
#define BIND 295
#define PRIVILEGED 296
#define EXTERNAL_PROTOCOL 297
#define INTERNAL_PROTOCOL 298
#define EXTERNAL_ROTATION 299
#define SAMESAME 300
#define GROUPNAME 301
#define HOSTID 302
#define HOSTINDEX 303
#define INTERFACE 304
#define SOCKETOPTION_SYMBOLICVALUE 305
#define INTERNAL 306
#define EXTERNAL 307
#define INTERNALSOCKET 308
#define EXTERNALSOCKET 309
#define IOTIMEOUT 310
#define IOTIMEOUT_TCP 311
#define IOTIMEOUT_UDP 312
#define NEGOTIATETIMEOUT 313
#define LIBWRAP_FILE 314
#define LOGLEVEL 315
#define SOCKSMETHOD 316
#define CLIENTMETHOD 317
#define METHOD 318
#define METHODNAME 319
#define NONE 320
#define BSDAUTH 321
#define GSSAPI 322
#define PAM_ADDRESS 323
#define PAM_ANY 324
#define PAM_USERNAME 325
#define RFC931 326
#define UNAME 327
#define MONITOR 328
#define PROCESSTYPE 329
#define PROC_MAXREQUESTS 330
#define PROC_MAXLIFETIME 331
#define REALM 332
#define REALNAME 333
#define RESOLVEPROTOCOL 334
#define REQUIRED 335
#define SCHEDULEPOLICY 336
#define SERVERCONFIG 337
#define CLIENTCONFIG 338
#define SOCKET 339
#define CLIENTSIDE_SOCKET 340
#define SNDBUF 341
#define RCVBUF 342
#define SOCKETPROTOCOL 343
#define SOCKETOPTION_OPTID 344
#define SRCHOST 345
#define NODNSMISMATCH 346
#define NODNSUNKNOWN 347
#define CHECKREPLYAUTH 348
#define USERNAME 349
#define USER_PRIVILEGED 350
#define USER_UNPRIVILEGED 351
#define USER_LIBWRAP 352
#define WORD__IN 353
#define ROUTE 354
#define VIA 355
#define GLOBALROUTEOPTION 356
#define BADROUTE_EXPIRE 357
#define MAXFAIL 358
#define PORT 359
#define NUMBER 360
#define BANDWIDTH 361
#define BOUNCE 362
#define BSDAUTHSTYLE 363
#define BSDAUTHSTYLENAME 364
#define COMMAND 365
#define COMMAND_BIND 366
#define COMMAND_CONNECT 367
#define COMMAND_UDPASSOCIATE 368
#define COMMAND_BINDREPLY 369
#define COMMAND_UDPREPLY 370
#define ACTION 371
#define FROM 372
#define TO 373
#define GSSAPIENCTYPE 374
#define GSSAPIENC_ANY 375
#define GSSAPIENC_CLEAR 376
#define GSSAPIENC_INTEGRITY 377
#define GSSAPIENC_CONFIDENTIALITY 378
#define GSSAPIENC_PERMESSAGE 379
#define GSSAPIKEYTAB 380
#define GSSAPISERVICE 381
#define GSSAPISERVICENAME 382
#define GSSAPIKEYTABNAME 383
#define IPV4 384
#define IPV6 385
#define IPVANY 386
#define DOMAINNAME 387
#define IFNAME 388
#define URL 389
#define LDAPATTRIBUTE 390
#define LDAPATTRIBUTE_AD 391
#define LDAPATTRIBUTE_HEX 392
#define LDAPATTRIBUTE_AD_HEX 393
#define LDAPBASEDN 394
#define LDAP_BASEDN 395
#define LDAPBASEDN_HEX 396
#define LDAPBASEDN_HEX_ALL 397
#define LDAPCERTFILE 398
#define LDAPCERTPATH 399
#define LDAPPORT 400
#define LDAPPORTSSL 401
#define LDAPDEBUG 402
#define LDAPDEPTH 403
#define LDAPAUTO 404
#define LDAPSEARCHTIME 405
#define LDAPDOMAIN 406
#define LDAP_DOMAIN 407
#define LDAPFILTER 408
#define LDAPFILTER_AD 409
#define LDAPFILTER_HEX 410
#define LDAPFILTER_AD_HEX 411
#define LDAPGROUP 412
#define LDAPGROUP_NAME 413
#define LDAPGROUP_HEX 414
#define LDAPGROUP_HEX_ALL 415
#define LDAPKEYTAB 416
#define LDAPKEYTABNAME 417
#define LDAPDEADTIME 418
#define LDAPSERVER 419
#define LDAPSERVER_NAME 420
#define LDAPAUTHSERVER 421
#define LDAPAUTHKEYTAB 422
#define LDAPSSL 423
#define LDAPCERTCHECK 424
#define LDAPKEEPREALM 425
#define LDAPTIMEOUT 426
#define LDAPCACHE 427
#define LDAPCACHEPOS 428
#define LDAPCACHENEG 429
#define LDAPURL 430
#define LDAP_URL 431
#define LDAPAUTHBASEDN 432
#define LDAPAUTHBASEDN_HEX 433
#define LDAPAUTHBASEDN_HEX_ALL 434
#define LDAPAUTHURL 435
#define LDAPAUTHPORT 436
#define LDAPAUTHPORTSSL 437
#define LDAPAUTHDEBUG 438
#define LDAPAUTHSSL 439
#define LDAPAUTHAUTO 440
#define LDAPAUTHCERTCHECK 441
#define LDAPAUTHFILTER 442
#define LDAPAUTHDOMAIN 443
#define LDAPAUTHCERTFILE 444
#define LDAPAUTHCERTPATH 445
#define LDAPAUTHKEEPREALM 446
#define LDAP_FILTER 447
#define LDAP_ATTRIBUTE 448
#define LDAP_CERTFILE 449
#define LDAP_CERTPATH 450
#define LIBWRAPSTART 451
#define LIBWRAP_ALLOW 452
#define LIBWRAP_DENY 453
#define LIBWRAP_HOSTS_ACCESS 454
#define LINE 455
#define OPERATOR 456
#define PACSID 457
#define PACSID_B64 458
#define PACSID_FLAG 459
#define PACSID_NAME 460
#define PAMSERVICENAME 461
#define PROTOCOL 462
#define PROTOCOL_TCP 463
#define PROTOCOL_UDP 464
#define PROTOCOL_FAKE 465
#define PROXYPROTOCOL 466
#define PROXYPROTOCOL_SOCKS_V4 467
#define PROXYPROTOCOL_SOCKS_V5 468
#define PROXYPROTOCOL_HTTP 469
#define PROXYPROTOCOL_UPNP 470
#define REDIRECT 471
#define SENDSIDE 472
#define RECVSIDE 473
#define SERVICENAME 474
#define SESSION_INHERITABLE 475
#define SESSIONMAX 476
#define SESSIONTHROTTLE 477
#define SESSIONSTATE_KEY 478
#define SESSIONSTATE_MAX 479
#define SESSIONSTATE_THROTTLE 480
#define RULE_LOG 481
#define RULE_LOG_CONNECT 482
#define RULE_LOG_DATA 483
#define RULE_LOG_DISCONNECT 484
#define RULE_LOG_ERROR 485
#define RULE_LOG_IOOPERATION 486
#define RULE_LOG_TCPINFO 487
#define STATEKEY 488
#define UDPPORTRANGE 489
#define UDPCONNECTDST 490
#define USER 491
#define GROUP 492
#define VERDICT_BLOCK 493
#define VERDICT_PASS 494
#define YES 495
#define NO 496
#define LOGFORMAT 497
#define LOGFORMAT_VALUE 498

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 418 "config_parse.y"

   struct {
      uid_t   uid;
      gid_t   gid;
   } uid;

   struct {
      valuetype_t valuetype;
      const int   *valuev;
   } error;

   struct {
      const char *oldname;
      const char *newname;
   } deprecated;

   char       *string;
   int        method;
   long long  number;

#line 1009 "config_parse.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE socks_yylval;


int socks_yyparse (void);


#endif /* !YY_SOCKS_YY_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ALARM = 3,                      /* ALARM  */
  YYSYMBOL_ALARMTYPE_DATA = 4,             /* ALARMTYPE_DATA  */
  YYSYMBOL_ALARMTYPE_DISCONNECT = 5,       /* ALARMTYPE_DISCONNECT  */
  YYSYMBOL_ALARMIF_INTERNAL = 6,           /* ALARMIF_INTERNAL  */
  YYSYMBOL_ALARMIF_EXTERNAL = 7,           /* ALARMIF_EXTERNAL  */
  YYSYMBOL_TCPOPTION_DISABLED = 8,         /* TCPOPTION_DISABLED  */
  YYSYMBOL_ECN = 9,                        /* ECN  */
  YYSYMBOL_SACK = 10,                      /* SACK  */
  YYSYMBOL_TIMESTAMPS = 11,                /* TIMESTAMPS  */
  YYSYMBOL_WSCALE = 12,                    /* WSCALE  */
  YYSYMBOL_MTU_ERROR = 13,                 /* MTU_ERROR  */
  YYSYMBOL_CLIENTCOMPATIBILITY = 14,       /* CLIENTCOMPATIBILITY  */
  YYSYMBOL_NECGSSAPI = 15,                 /* NECGSSAPI  */
  YYSYMBOL_CLIENTRULE = 16,                /* CLIENTRULE  */
  YYSYMBOL_HOSTIDRULE = 17,                /* HOSTIDRULE  */
  YYSYMBOL_SOCKSRULE = 18,                 /* SOCKSRULE  */
  YYSYMBOL_COMPATIBILITY = 19,             /* COMPATIBILITY  */
  YYSYMBOL_SAMEPORT = 20,                  /* SAMEPORT  */
  YYSYMBOL_DRAFT_5_05 = 21,                /* DRAFT_5_05  */
  YYSYMBOL_CONNECTTIMEOUT = 22,            /* CONNECTTIMEOUT  */
  YYSYMBOL_TCP_FIN_WAIT = 23,              /* TCP_FIN_WAIT  */
  YYSYMBOL_CPU = 24,                       /* CPU  */
  YYSYMBOL_MASK = 25,                      /* MASK  */
  YYSYMBOL_SCHEDULE = 26,                  /* SCHEDULE  */
  YYSYMBOL_CPUMASK_ANYCPU = 27,            /* CPUMASK_ANYCPU  */
  YYSYMBOL_DEBUGGING = 28,                 /* DEBUGGING  */
  YYSYMBOL_DEPRECATED = 29,                /* DEPRECATED  */
  YYSYMBOL_ERRORLOG = 30,                  /* ERRORLOG  */
  YYSYMBOL_LOGOUTPUT = 31,                 /* LOGOUTPUT  */
  YYSYMBOL_LOGFILE = 32,                   /* LOGFILE  */
  YYSYMBOL_LOGTYPE_ERROR = 33,             /* LOGTYPE_ERROR  */
  YYSYMBOL_LOGTYPE_TCP_DISABLED = 34,      /* LOGTYPE_TCP_DISABLED  */
  YYSYMBOL_LOGTYPE_TCP_ENABLED = 35,       /* LOGTYPE_TCP_ENABLED  */
  YYSYMBOL_LOGIF_INTERNAL = 36,            /* LOGIF_INTERNAL  */
  YYSYMBOL_LOGIF_EXTERNAL = 37,            /* LOGIF_EXTERNAL  */
  YYSYMBOL_ERRORVALUE = 38,                /* ERRORVALUE  */
  YYSYMBOL_EXTENSION = 39,                 /* EXTENSION  */
  YYSYMBOL_BIND = 40,                      /* BIND  */
  YYSYMBOL_PRIVILEGED = 41,                /* PRIVILEGED  */
  YYSYMBOL_EXTERNAL_PROTOCOL = 42,         /* EXTERNAL_PROTOCOL  */
  YYSYMBOL_INTERNAL_PROTOCOL = 43,         /* INTERNAL_PROTOCOL  */
  YYSYMBOL_EXTERNAL_ROTATION = 44,         /* EXTERNAL_ROTATION  */
  YYSYMBOL_SAMESAME = 45,                  /* SAMESAME  */
  YYSYMBOL_GROUPNAME = 46,                 /* GROUPNAME  */
  YYSYMBOL_HOSTID = 47,                    /* HOSTID  */
  YYSYMBOL_HOSTINDEX = 48,                 /* HOSTINDEX  */
  YYSYMBOL_INTERFACE = 49,                 /* INTERFACE  */
  YYSYMBOL_SOCKETOPTION_SYMBOLICVALUE = 50, /* SOCKETOPTION_SYMBOLICVALUE  */
  YYSYMBOL_INTERNAL = 51,                  /* INTERNAL  */
  YYSYMBOL_EXTERNAL = 52,                  /* EXTERNAL  */
  YYSYMBOL_INTERNALSOCKET = 53,            /* INTERNALSOCKET  */
  YYSYMBOL_EXTERNALSOCKET = 54,            /* EXTERNALSOCKET  */
  YYSYMBOL_IOTIMEOUT = 55,                 /* IOTIMEOUT  */
  YYSYMBOL_IOTIMEOUT_TCP = 56,             /* IOTIMEOUT_TCP  */
  YYSYMBOL_IOTIMEOUT_UDP = 57,             /* IOTIMEOUT_UDP  */
  YYSYMBOL_NEGOTIATETIMEOUT = 58,          /* NEGOTIATETIMEOUT  */
  YYSYMBOL_LIBWRAP_FILE = 59,              /* LIBWRAP_FILE  */
  YYSYMBOL_LOGLEVEL = 60,                  /* LOGLEVEL  */
  YYSYMBOL_SOCKSMETHOD = 61,               /* SOCKSMETHOD  */
  YYSYMBOL_CLIENTMETHOD = 62,              /* CLIENTMETHOD  */
  YYSYMBOL_METHOD = 63,                    /* METHOD  */
  YYSYMBOL_METHODNAME = 64,                /* METHODNAME  */
  YYSYMBOL_NONE = 65,                      /* NONE  */
  YYSYMBOL_BSDAUTH = 66,                   /* BSDAUTH  */
  YYSYMBOL_GSSAPI = 67,                    /* GSSAPI  */
  YYSYMBOL_PAM_ADDRESS = 68,               /* PAM_ADDRESS  */
  YYSYMBOL_PAM_ANY = 69,                   /* PAM_ANY  */
  YYSYMBOL_PAM_USERNAME = 70,              /* PAM_USERNAME  */
  YYSYMBOL_RFC931 = 71,                    /* RFC931  */
  YYSYMBOL_UNAME = 72,                     /* UNAME  */
  YYSYMBOL_MONITOR = 73,                   /* MONITOR  */
  YYSYMBOL_PROCESSTYPE = 74,               /* PROCESSTYPE  */
  YYSYMBOL_PROC_MAXREQUESTS = 75,          /* PROC_MAXREQUESTS  */
  YYSYMBOL_PROC_MAXLIFETIME = 76,          /* PROC_MAXLIFETIME  */
  YYSYMBOL_REALM = 77,                     /* REALM  */
  YYSYMBOL_REALNAME = 78,                  /* REALNAME  */
  YYSYMBOL_RESOLVEPROTOCOL = 79,           /* RESOLVEPROTOCOL  */
  YYSYMBOL_REQUIRED = 80,                  /* REQUIRED  */
  YYSYMBOL_SCHEDULEPOLICY = 81,            /* SCHEDULEPOLICY  */
  YYSYMBOL_SERVERCONFIG = 82,              /* SERVERCONFIG  */
  YYSYMBOL_CLIENTCONFIG = 83,              /* CLIENTCONFIG  */
  YYSYMBOL_SOCKET = 84,                    /* SOCKET  */
  YYSYMBOL_CLIENTSIDE_SOCKET = 85,         /* CLIENTSIDE_SOCKET  */
  YYSYMBOL_SNDBUF = 86,                    /* SNDBUF  */
  YYSYMBOL_RCVBUF = 87,                    /* RCVBUF  */
  YYSYMBOL_SOCKETPROTOCOL = 88,            /* SOCKETPROTOCOL  */
  YYSYMBOL_SOCKETOPTION_OPTID = 89,        /* SOCKETOPTION_OPTID  */
  YYSYMBOL_SRCHOST = 90,                   /* SRCHOST  */
  YYSYMBOL_NODNSMISMATCH = 91,             /* NODNSMISMATCH  */
  YYSYMBOL_NODNSUNKNOWN = 92,              /* NODNSUNKNOWN  */
  YYSYMBOL_CHECKREPLYAUTH = 93,            /* CHECKREPLYAUTH  */
  YYSYMBOL_USERNAME = 94,                  /* USERNAME  */
  YYSYMBOL_USER_PRIVILEGED = 95,           /* USER_PRIVILEGED  */
  YYSYMBOL_USER_UNPRIVILEGED = 96,         /* USER_UNPRIVILEGED  */
  YYSYMBOL_USER_LIBWRAP = 97,              /* USER_LIBWRAP  */
  YYSYMBOL_WORD__IN = 98,                  /* WORD__IN  */
  YYSYMBOL_ROUTE = 99,                     /* ROUTE  */
  YYSYMBOL_VIA = 100,                      /* VIA  */
  YYSYMBOL_GLOBALROUTEOPTION = 101,        /* GLOBALROUTEOPTION  */
  YYSYMBOL_BADROUTE_EXPIRE = 102,          /* BADROUTE_EXPIRE  */
  YYSYMBOL_MAXFAIL = 103,                  /* MAXFAIL  */
  YYSYMBOL_PORT = 104,                     /* PORT  */
  YYSYMBOL_NUMBER = 105,                   /* NUMBER  */
  YYSYMBOL_BANDWIDTH = 106,                /* BANDWIDTH  */
  YYSYMBOL_BOUNCE = 107,                   /* BOUNCE  */
  YYSYMBOL_BSDAUTHSTYLE = 108,             /* BSDAUTHSTYLE  */
  YYSYMBOL_BSDAUTHSTYLENAME = 109,         /* BSDAUTHSTYLENAME  */
  YYSYMBOL_COMMAND = 110,                  /* COMMAND  */
  YYSYMBOL_COMMAND_BIND = 111,             /* COMMAND_BIND  */
  YYSYMBOL_COMMAND_CONNECT = 112,          /* COMMAND_CONNECT  */
  YYSYMBOL_COMMAND_UDPASSOCIATE = 113,     /* COMMAND_UDPASSOCIATE  */
  YYSYMBOL_COMMAND_BINDREPLY = 114,        /* COMMAND_BINDREPLY  */
  YYSYMBOL_COMMAND_UDPREPLY = 115,         /* COMMAND_UDPREPLY  */
  YYSYMBOL_ACTION = 116,                   /* ACTION  */
  YYSYMBOL_FROM = 117,                     /* FROM  */
  YYSYMBOL_TO = 118,                       /* TO  */
  YYSYMBOL_GSSAPIENCTYPE = 119,            /* GSSAPIENCTYPE  */
  YYSYMBOL_GSSAPIENC_ANY = 120,            /* GSSAPIENC_ANY  */
  YYSYMBOL_GSSAPIENC_CLEAR = 121,          /* GSSAPIENC_CLEAR  */
  YYSYMBOL_GSSAPIENC_INTEGRITY = 122,      /* GSSAPIENC_INTEGRITY  */
  YYSYMBOL_GSSAPIENC_CONFIDENTIALITY = 123, /* GSSAPIENC_CONFIDENTIALITY  */
  YYSYMBOL_GSSAPIENC_PERMESSAGE = 124,     /* GSSAPIENC_PERMESSAGE  */
  YYSYMBOL_GSSAPIKEYTAB = 125,             /* GSSAPIKEYTAB  */
  YYSYMBOL_GSSAPISERVICE = 126,            /* GSSAPISERVICE  */
  YYSYMBOL_GSSAPISERVICENAME = 127,        /* GSSAPISERVICENAME  */
  YYSYMBOL_GSSAPIKEYTABNAME = 128,         /* GSSAPIKEYTABNAME  */
  YYSYMBOL_IPV4 = 129,                     /* IPV4  */
  YYSYMBOL_IPV6 = 130,                     /* IPV6  */
  YYSYMBOL_IPVANY = 131,                   /* IPVANY  */
  YYSYMBOL_DOMAINNAME = 132,               /* DOMAINNAME  */
  YYSYMBOL_IFNAME = 133,                   /* IFNAME  */
  YYSYMBOL_URL = 134,                      /* URL  */
  YYSYMBOL_LDAPATTRIBUTE = 135,            /* LDAPATTRIBUTE  */
  YYSYMBOL_LDAPATTRIBUTE_AD = 136,         /* LDAPATTRIBUTE_AD  */
  YYSYMBOL_LDAPATTRIBUTE_HEX = 137,        /* LDAPATTRIBUTE_HEX  */
  YYSYMBOL_LDAPATTRIBUTE_AD_HEX = 138,     /* LDAPATTRIBUTE_AD_HEX  */
  YYSYMBOL_LDAPBASEDN = 139,               /* LDAPBASEDN  */
  YYSYMBOL_LDAP_BASEDN = 140,              /* LDAP_BASEDN  */
  YYSYMBOL_LDAPBASEDN_HEX = 141,           /* LDAPBASEDN_HEX  */
  YYSYMBOL_LDAPBASEDN_HEX_ALL = 142,       /* LDAPBASEDN_HEX_ALL  */
  YYSYMBOL_LDAPCERTFILE = 143,             /* LDAPCERTFILE  */
  YYSYMBOL_LDAPCERTPATH = 144,             /* LDAPCERTPATH  */
  YYSYMBOL_LDAPPORT = 145,                 /* LDAPPORT  */
  YYSYMBOL_LDAPPORTSSL = 146,              /* LDAPPORTSSL  */
  YYSYMBOL_LDAPDEBUG = 147,                /* LDAPDEBUG  */
  YYSYMBOL_LDAPDEPTH = 148,                /* LDAPDEPTH  */
  YYSYMBOL_LDAPAUTO = 149,                 /* LDAPAUTO  */
  YYSYMBOL_LDAPSEARCHTIME = 150,           /* LDAPSEARCHTIME  */
  YYSYMBOL_LDAPDOMAIN = 151,               /* LDAPDOMAIN  */
  YYSYMBOL_LDAP_DOMAIN = 152,              /* LDAP_DOMAIN  */
  YYSYMBOL_LDAPFILTER = 153,               /* LDAPFILTER  */
  YYSYMBOL_LDAPFILTER_AD = 154,            /* LDAPFILTER_AD  */
  YYSYMBOL_LDAPFILTER_HEX = 155,           /* LDAPFILTER_HEX  */
  YYSYMBOL_LDAPFILTER_AD_HEX = 156,        /* LDAPFILTER_AD_HEX  */
  YYSYMBOL_LDAPGROUP = 157,                /* LDAPGROUP  */
  YYSYMBOL_LDAPGROUP_NAME = 158,           /* LDAPGROUP_NAME  */
  YYSYMBOL_LDAPGROUP_HEX = 159,            /* LDAPGROUP_HEX  */
  YYSYMBOL_LDAPGROUP_HEX_ALL = 160,        /* LDAPGROUP_HEX_ALL  */
  YYSYMBOL_LDAPKEYTAB = 161,               /* LDAPKEYTAB  */
  YYSYMBOL_LDAPKEYTABNAME = 162,           /* LDAPKEYTABNAME  */
  YYSYMBOL_LDAPDEADTIME = 163,             /* LDAPDEADTIME  */
  YYSYMBOL_LDAPSERVER = 164,               /* LDAPSERVER  */
  YYSYMBOL_LDAPSERVER_NAME = 165,          /* LDAPSERVER_NAME  */
  YYSYMBOL_LDAPAUTHSERVER = 166,           /* LDAPAUTHSERVER  */
  YYSYMBOL_LDAPAUTHKEYTAB = 167,           /* LDAPAUTHKEYTAB  */
  YYSYMBOL_LDAPSSL = 168,                  /* LDAPSSL  */
  YYSYMBOL_LDAPCERTCHECK = 169,            /* LDAPCERTCHECK  */
  YYSYMBOL_LDAPKEEPREALM = 170,            /* LDAPKEEPREALM  */
  YYSYMBOL_LDAPTIMEOUT = 171,              /* LDAPTIMEOUT  */
  YYSYMBOL_LDAPCACHE = 172,                /* LDAPCACHE  */
  YYSYMBOL_LDAPCACHEPOS = 173,             /* LDAPCACHEPOS  */
  YYSYMBOL_LDAPCACHENEG = 174,             /* LDAPCACHENEG  */
  YYSYMBOL_LDAPURL = 175,                  /* LDAPURL  */
  YYSYMBOL_LDAP_URL = 176,                 /* LDAP_URL  */
  YYSYMBOL_LDAPAUTHBASEDN = 177,           /* LDAPAUTHBASEDN  */
  YYSYMBOL_LDAPAUTHBASEDN_HEX = 178,       /* LDAPAUTHBASEDN_HEX  */
  YYSYMBOL_LDAPAUTHBASEDN_HEX_ALL = 179,   /* LDAPAUTHBASEDN_HEX_ALL  */
  YYSYMBOL_LDAPAUTHURL = 180,              /* LDAPAUTHURL  */
  YYSYMBOL_LDAPAUTHPORT = 181,             /* LDAPAUTHPORT  */
  YYSYMBOL_LDAPAUTHPORTSSL = 182,          /* LDAPAUTHPORTSSL  */
  YYSYMBOL_LDAPAUTHDEBUG = 183,            /* LDAPAUTHDEBUG  */
  YYSYMBOL_LDAPAUTHSSL = 184,              /* LDAPAUTHSSL  */
  YYSYMBOL_LDAPAUTHAUTO = 185,             /* LDAPAUTHAUTO  */
  YYSYMBOL_LDAPAUTHCERTCHECK = 186,        /* LDAPAUTHCERTCHECK  */
  YYSYMBOL_LDAPAUTHFILTER = 187,           /* LDAPAUTHFILTER  */
  YYSYMBOL_LDAPAUTHDOMAIN = 188,           /* LDAPAUTHDOMAIN  */
  YYSYMBOL_LDAPAUTHCERTFILE = 189,         /* LDAPAUTHCERTFILE  */
  YYSYMBOL_LDAPAUTHCERTPATH = 190,         /* LDAPAUTHCERTPATH  */
  YYSYMBOL_LDAPAUTHKEEPREALM = 191,        /* LDAPAUTHKEEPREALM  */
  YYSYMBOL_LDAP_FILTER = 192,              /* LDAP_FILTER  */
  YYSYMBOL_LDAP_ATTRIBUTE = 193,           /* LDAP_ATTRIBUTE  */
  YYSYMBOL_LDAP_CERTFILE = 194,            /* LDAP_CERTFILE  */
  YYSYMBOL_LDAP_CERTPATH = 195,            /* LDAP_CERTPATH  */
  YYSYMBOL_LIBWRAPSTART = 196,             /* LIBWRAPSTART  */
  YYSYMBOL_LIBWRAP_ALLOW = 197,            /* LIBWRAP_ALLOW  */
  YYSYMBOL_LIBWRAP_DENY = 198,             /* LIBWRAP_DENY  */
  YYSYMBOL_LIBWRAP_HOSTS_ACCESS = 199,     /* LIBWRAP_HOSTS_ACCESS  */
  YYSYMBOL_LINE = 200,                     /* LINE  */
  YYSYMBOL_OPERATOR = 201,                 /* OPERATOR  */
  YYSYMBOL_PACSID = 202,                   /* PACSID  */
  YYSYMBOL_PACSID_B64 = 203,               /* PACSID_B64  */
  YYSYMBOL_PACSID_FLAG = 204,              /* PACSID_FLAG  */
  YYSYMBOL_PACSID_NAME = 205,              /* PACSID_NAME  */
  YYSYMBOL_PAMSERVICENAME = 206,           /* PAMSERVICENAME  */
  YYSYMBOL_PROTOCOL = 207,                 /* PROTOCOL  */
  YYSYMBOL_PROTOCOL_TCP = 208,             /* PROTOCOL_TCP  */
  YYSYMBOL_PROTOCOL_UDP = 209,             /* PROTOCOL_UDP  */
  YYSYMBOL_PROTOCOL_FAKE = 210,            /* PROTOCOL_FAKE  */
  YYSYMBOL_PROXYPROTOCOL = 211,            /* PROXYPROTOCOL  */
  YYSYMBOL_PROXYPROTOCOL_SOCKS_V4 = 212,   /* PROXYPROTOCOL_SOCKS_V4  */
  YYSYMBOL_PROXYPROTOCOL_SOCKS_V5 = 213,   /* PROXYPROTOCOL_SOCKS_V5  */
  YYSYMBOL_PROXYPROTOCOL_HTTP = 214,       /* PROXYPROTOCOL_HTTP  */
  YYSYMBOL_PROXYPROTOCOL_UPNP = 215,       /* PROXYPROTOCOL_UPNP  */
  YYSYMBOL_REDIRECT = 216,                 /* REDIRECT  */
  YYSYMBOL_SENDSIDE = 217,                 /* SENDSIDE  */
  YYSYMBOL_RECVSIDE = 218,                 /* RECVSIDE  */
  YYSYMBOL_SERVICENAME = 219,              /* SERVICENAME  */
  YYSYMBOL_SESSION_INHERITABLE = 220,      /* SESSION_INHERITABLE  */
  YYSYMBOL_SESSIONMAX = 221,               /* SESSIONMAX  */
  YYSYMBOL_SESSIONTHROTTLE = 222,          /* SESSIONTHROTTLE  */
  YYSYMBOL_SESSIONSTATE_KEY = 223,         /* SESSIONSTATE_KEY  */
  YYSYMBOL_SESSIONSTATE_MAX = 224,         /* SESSIONSTATE_MAX  */
  YYSYMBOL_SESSIONSTATE_THROTTLE = 225,    /* SESSIONSTATE_THROTTLE  */
  YYSYMBOL_RULE_LOG = 226,                 /* RULE_LOG  */
  YYSYMBOL_RULE_LOG_CONNECT = 227,         /* RULE_LOG_CONNECT  */
  YYSYMBOL_RULE_LOG_DATA = 228,            /* RULE_LOG_DATA  */
  YYSYMBOL_RULE_LOG_DISCONNECT = 229,      /* RULE_LOG_DISCONNECT  */
  YYSYMBOL_RULE_LOG_ERROR = 230,           /* RULE_LOG_ERROR  */
  YYSYMBOL_RULE_LOG_IOOPERATION = 231,     /* RULE_LOG_IOOPERATION  */
  YYSYMBOL_RULE_LOG_TCPINFO = 232,         /* RULE_LOG_TCPINFO  */
  YYSYMBOL_STATEKEY = 233,                 /* STATEKEY  */
  YYSYMBOL_UDPPORTRANGE = 234,             /* UDPPORTRANGE  */
  YYSYMBOL_UDPCONNECTDST = 235,            /* UDPCONNECTDST  */
  YYSYMBOL_USER = 236,                     /* USER  */
  YYSYMBOL_GROUP = 237,                    /* GROUP  */
  YYSYMBOL_VERDICT_BLOCK = 238,            /* VERDICT_BLOCK  */
  YYSYMBOL_VERDICT_PASS = 239,             /* VERDICT_PASS  */
  YYSYMBOL_YES = 240,                      /* YES  */
  YYSYMBOL_NO = 241,                       /* NO  */
  YYSYMBOL_LOGFORMAT = 242,                /* LOGFORMAT  */
  YYSYMBOL_LOGFORMAT_VALUE = 243,          /* LOGFORMAT_VALUE  */
  YYSYMBOL_244_ = 244,                     /* ':'  */
  YYSYMBOL_245_ = 245,                     /* '.'  */
  YYSYMBOL_246_ = 246,                     /* '{'  */
  YYSYMBOL_247_ = 247,                     /* '}'  */
  YYSYMBOL_248_ = 248,                     /* '/'  */
  YYSYMBOL_249_ = 249,                     /* '-'  */
  YYSYMBOL_YYACCEPT = 250,                 /* $accept  */
  YYSYMBOL_configtype = 251,               /* configtype  */
  YYSYMBOL_252_1 = 252,                    /* $@1  */
  YYSYMBOL_serverobjects = 253,            /* serverobjects  */
  YYSYMBOL_serverobject = 254,             /* serverobject  */
  YYSYMBOL_serveroptions = 255,            /* serveroptions  */
  YYSYMBOL_serveroption = 256,             /* serveroption  */
  YYSYMBOL_logspecial = 257,               /* logspecial  */
  YYSYMBOL_258_2 = 258,                    /* $@2  */
  YYSYMBOL_259_3 = 259,                    /* $@3  */
  YYSYMBOL_internal_if_logoption = 260,    /* internal_if_logoption  */
  YYSYMBOL_261_4 = 261,                    /* $@4  */
  YYSYMBOL_external_if_logoption = 262,    /* external_if_logoption  */
  YYSYMBOL_263_5 = 263,                    /* $@5  */
  YYSYMBOL_rule_internal_logoption = 264,  /* rule_internal_logoption  */
  YYSYMBOL_265_6 = 265,                    /* $@6  */
  YYSYMBOL_rule_external_logoption = 266,  /* rule_external_logoption  */
  YYSYMBOL_267_7 = 267,                    /* $@7  */
  YYSYMBOL_loglevel = 268,                 /* loglevel  */
  YYSYMBOL_tcpoptions = 269,               /* tcpoptions  */
  YYSYMBOL_tcpoption = 270,                /* tcpoption  */
  YYSYMBOL_errors = 271,                   /* errors  */
  YYSYMBOL_errorobject = 272,              /* errorobject  */
  YYSYMBOL_timeout = 273,                  /* timeout  */
  YYSYMBOL_deprecated = 274,               /* deprecated  */
  YYSYMBOL_route = 275,                    /* route  */
  YYSYMBOL_276_8 = 276,                    /* $@8  */
  YYSYMBOL_277_9 = 277,                    /* $@9  */
  YYSYMBOL_routes = 278,                   /* routes  */
  YYSYMBOL_proxyprotocol = 279,            /* proxyprotocol  */
  YYSYMBOL_proxyprotocolname = 280,        /* proxyprotocolname  */
  YYSYMBOL_proxyprotocols = 281,           /* proxyprotocols  */
  YYSYMBOL_user = 282,                     /* user  */
  YYSYMBOL_username = 283,                 /* username  */
  YYSYMBOL_usernames = 284,                /* usernames  */
  YYSYMBOL_group = 285,                    /* group  */
  YYSYMBOL_groupname = 286,                /* groupname  */
  YYSYMBOL_groupnames = 287,               /* groupnames  */
  YYSYMBOL_extension = 288,                /* extension  */
  YYSYMBOL_extensionname = 289,            /* extensionname  */
  YYSYMBOL_extensions = 290,               /* extensions  */
  YYSYMBOL_ifprotocols = 291,              /* ifprotocols  */
  YYSYMBOL_ifprotocol = 292,               /* ifprotocol  */
  YYSYMBOL_internal = 293,                 /* internal  */
  YYSYMBOL_internalinit = 294,             /* internalinit  */
  YYSYMBOL_internal_protocol = 295,        /* internal_protocol  */
  YYSYMBOL_296_10 = 296,                   /* $@10  */
  YYSYMBOL_external = 297,                 /* external  */
  YYSYMBOL_externalinit = 298,             /* externalinit  */
  YYSYMBOL_external_protocol = 299,        /* external_protocol  */
  YYSYMBOL_300_11 = 300,                   /* $@11  */
  YYSYMBOL_external_rotation = 301,        /* external_rotation  */
  YYSYMBOL_clientoption = 302,             /* clientoption  */
  YYSYMBOL_clientoptions = 303,            /* clientoptions  */
  YYSYMBOL_global_routeoption = 304,       /* global_routeoption  */
  YYSYMBOL_logformat = 305,                /* logformat  */
  YYSYMBOL_errorlog = 306,                 /* errorlog  */
  YYSYMBOL_307_12 = 307,                   /* $@12  */
  YYSYMBOL_logoutput = 308,                /* logoutput  */
  YYSYMBOL_309_13 = 309,                   /* $@13  */
  YYSYMBOL_logoutputdevice = 310,          /* logoutputdevice  */
  YYSYMBOL_logoutputdevices = 311,         /* logoutputdevices  */
  YYSYMBOL_childstate = 312,               /* childstate  */
  YYSYMBOL_userids = 313,                  /* userids  */
  YYSYMBOL_user_privileged = 314,          /* user_privileged  */
  YYSYMBOL_user_unprivileged = 315,        /* user_unprivileged  */
  YYSYMBOL_user_libwrap = 316,             /* user_libwrap  */
  YYSYMBOL_userid = 317,                   /* userid  */
  YYSYMBOL_iotimeout = 318,                /* iotimeout  */
  YYSYMBOL_negotiatetimeout = 319,         /* negotiatetimeout  */
  YYSYMBOL_connecttimeout = 320,           /* connecttimeout  */
  YYSYMBOL_tcp_fin_timeout = 321,          /* tcp_fin_timeout  */
  YYSYMBOL_debugging = 322,                /* debugging  */
  YYSYMBOL_libwrapfiles = 323,             /* libwrapfiles  */
  YYSYMBOL_libwrap_allowfile = 324,        /* libwrap_allowfile  */
  YYSYMBOL_libwrap_denyfile = 325,         /* libwrap_denyfile  */
  YYSYMBOL_libwrap_hosts_access = 326,     /* libwrap_hosts_access  */
  YYSYMBOL_udpconnectdst = 327,            /* udpconnectdst  */
  YYSYMBOL_compatibility = 328,            /* compatibility  */
  YYSYMBOL_compatibilityname = 329,        /* compatibilityname  */
  YYSYMBOL_compatibilitynames = 330,       /* compatibilitynames  */
  YYSYMBOL_resolveprotocol = 331,          /* resolveprotocol  */
  YYSYMBOL_resolveprotocolname = 332,      /* resolveprotocolname  */
  YYSYMBOL_cpu = 333,                      /* cpu  */
  YYSYMBOL_cpuschedule = 334,              /* cpuschedule  */
  YYSYMBOL_cpuaffinity = 335,              /* cpuaffinity  */
  YYSYMBOL_socketoption = 336,             /* socketoption  */
  YYSYMBOL_337_14 = 337,                   /* $@14  */
  YYSYMBOL_socketoptionname = 338,         /* socketoptionname  */
  YYSYMBOL_socketoptionvalue = 339,        /* socketoptionvalue  */
  YYSYMBOL_socketside = 340,               /* socketside  */
  YYSYMBOL_srchost = 341,                  /* srchost  */
  YYSYMBOL_srchostoption = 342,            /* srchostoption  */
  YYSYMBOL_srchostoptions = 343,           /* srchostoptions  */
  YYSYMBOL_realm = 344,                    /* realm  */
  YYSYMBOL_global_clientmethod = 345,      /* global_clientmethod  */
  YYSYMBOL_346_15 = 346,                   /* $@15  */
  YYSYMBOL_global_socksmethod = 347,       /* global_socksmethod  */
  YYSYMBOL_348_16 = 348,                   /* $@16  */
  YYSYMBOL_socksmethod = 349,              /* socksmethod  */
  YYSYMBOL_socksmethods = 350,             /* socksmethods  */
  YYSYMBOL_socksmethodname = 351,          /* socksmethodname  */
  YYSYMBOL_clientmethod = 352,             /* clientmethod  */
  YYSYMBOL_clientmethods = 353,            /* clientmethods  */
  YYSYMBOL_clientmethodname = 354,         /* clientmethodname  */
  YYSYMBOL_monitor = 355,                  /* monitor  */
  YYSYMBOL_356_17 = 356,                   /* $@17  */
  YYSYMBOL_357_18 = 357,                   /* $@18  */
  YYSYMBOL_crule = 358,                    /* crule  */
  YYSYMBOL_359_19 = 359,                   /* $@19  */
  YYSYMBOL_alarm = 360,                    /* alarm  */
  YYSYMBOL_monitorside = 361,              /* monitorside  */
  YYSYMBOL_alarmside = 362,                /* alarmside  */
  YYSYMBOL_alarm_data = 363,               /* alarm_data  */
  YYSYMBOL_364_20 = 364,                   /* $@20  */
  YYSYMBOL_alarm_test = 365,               /* alarm_test  */
  YYSYMBOL_networkproblem = 366,           /* networkproblem  */
  YYSYMBOL_alarm_disconnect = 367,         /* alarm_disconnect  */
  YYSYMBOL_alarmperiod = 368,              /* alarmperiod  */
  YYSYMBOL_monitoroption = 369,            /* monitoroption  */
  YYSYMBOL_monitoroptions = 370,           /* monitoroptions  */
  YYSYMBOL_cruleoption = 371,              /* cruleoption  */
  YYSYMBOL_hrule = 372,                    /* hrule  */
  YYSYMBOL_373_21 = 373,                   /* $@21  */
  YYSYMBOL_cruleoptions = 374,             /* cruleoptions  */
  YYSYMBOL_hostidoption = 375,             /* hostidoption  */
  YYSYMBOL_hostid = 376,                   /* hostid  */
  YYSYMBOL_377_22 = 377,                   /* $@22  */
  YYSYMBOL_hostindex = 378,                /* hostindex  */
  YYSYMBOL_srule = 379,                    /* srule  */
  YYSYMBOL_380_23 = 380,                   /* $@23  */
  YYSYMBOL_sruleoptions = 381,             /* sruleoptions  */
  YYSYMBOL_sruleoption = 382,              /* sruleoption  */
  YYSYMBOL_genericruleoption = 383,        /* genericruleoption  */
  YYSYMBOL_ldapauthoption = 384,           /* ldapauthoption  */
  YYSYMBOL_ldapoption = 385,               /* ldapoption  */
  YYSYMBOL_ldapdebug = 386,                /* ldapdebug  */
  YYSYMBOL_ldapauthdebug = 387,            /* ldapauthdebug  */
  YYSYMBOL_ldapdomain = 388,               /* ldapdomain  */
  YYSYMBOL_ldapauthdomain = 389,           /* ldapauthdomain  */
  YYSYMBOL_ldapdepth = 390,                /* ldapdepth  */
  YYSYMBOL_ldapcertfile = 391,             /* ldapcertfile  */
  YYSYMBOL_ldapauthcertfile = 392,         /* ldapauthcertfile  */
  YYSYMBOL_ldapcertpath = 393,             /* ldapcertpath  */
  YYSYMBOL_ldapauthcertpath = 394,         /* ldapauthcertpath  */
  YYSYMBOL_ldapurl = 395,                  /* ldapurl  */
  YYSYMBOL_ldapauthurl = 396,              /* ldapauthurl  */
  YYSYMBOL_ldapauthbasedn = 397,           /* ldapauthbasedn  */
  YYSYMBOL_ldapauthbasedn_hex = 398,       /* ldapauthbasedn_hex  */
  YYSYMBOL_ldapauthbasedn_hex_all = 399,   /* ldapauthbasedn_hex_all  */
  YYSYMBOL_lbasedn = 400,                  /* lbasedn  */
  YYSYMBOL_lbasedn_hex = 401,              /* lbasedn_hex  */
  YYSYMBOL_lbasedn_hex_all = 402,          /* lbasedn_hex_all  */
  YYSYMBOL_ldapauthport = 403,             /* ldapauthport  */
  YYSYMBOL_ldapport = 404,                 /* ldapport  */
  YYSYMBOL_ldapauthportssl = 405,          /* ldapauthportssl  */
  YYSYMBOL_ldapportssl = 406,              /* ldapportssl  */
  YYSYMBOL_ldapssl = 407,                  /* ldapssl  */
  YYSYMBOL_ldapauthssl = 408,              /* ldapauthssl  */
  YYSYMBOL_ldapauto = 409,                 /* ldapauto  */
  YYSYMBOL_ldapauthauto = 410,             /* ldapauthauto  */
  YYSYMBOL_ldapcertcheck = 411,            /* ldapcertcheck  */
  YYSYMBOL_ldapauthcertcheck = 412,        /* ldapauthcertcheck  */
  YYSYMBOL_ldapauthkeeprealm = 413,        /* ldapauthkeeprealm  */
  YYSYMBOL_ldapkeeprealm = 414,            /* ldapkeeprealm  */
  YYSYMBOL_ldapfilter = 415,               /* ldapfilter  */
  YYSYMBOL_ldapauthfilter = 416,           /* ldapauthfilter  */
  YYSYMBOL_ldapfilter_ad = 417,            /* ldapfilter_ad  */
  YYSYMBOL_ldapfilter_hex = 418,           /* ldapfilter_hex  */
  YYSYMBOL_ldapfilter_ad_hex = 419,        /* ldapfilter_ad_hex  */
  YYSYMBOL_ldapattribute = 420,            /* ldapattribute  */
  YYSYMBOL_ldapattribute_ad = 421,         /* ldapattribute_ad  */
  YYSYMBOL_ldapattribute_hex = 422,        /* ldapattribute_hex  */
  YYSYMBOL_ldapattribute_ad_hex = 423,     /* ldapattribute_ad_hex  */
  YYSYMBOL_lgroup_hex = 424,               /* lgroup_hex  */
  YYSYMBOL_lgroup_hex_all = 425,           /* lgroup_hex_all  */
  YYSYMBOL_lgroup = 426,                   /* lgroup  */
  YYSYMBOL_lserver = 427,                  /* lserver  */
  YYSYMBOL_ldapauthserver = 428,           /* ldapauthserver  */
  YYSYMBOL_ldapkeytab = 429,               /* ldapkeytab  */
  YYSYMBOL_ldapauthkeytab = 430,           /* ldapauthkeytab  */
  YYSYMBOL_psid = 431,                     /* psid  */
  YYSYMBOL_psid_b64 = 432,                 /* psid_b64  */
  YYSYMBOL_psid_off = 433,                 /* psid_off  */
  YYSYMBOL_clientcompatibility = 434,      /* clientcompatibility  */
  YYSYMBOL_clientcompatibilityname = 435,  /* clientcompatibilityname  */
  YYSYMBOL_clientcompatibilitynames = 436, /* clientcompatibilitynames  */
  YYSYMBOL_verdict = 437,                  /* verdict  */
  YYSYMBOL_command = 438,                  /* command  */
  YYSYMBOL_commands = 439,                 /* commands  */
  YYSYMBOL_commandname = 440,              /* commandname  */
  YYSYMBOL_protocol = 441,                 /* protocol  */
  YYSYMBOL_protocols = 442,                /* protocols  */
  YYSYMBOL_protocolname = 443,             /* protocolname  */
  YYSYMBOL_fromto = 444,                   /* fromto  */
  YYSYMBOL_hostid_fromto = 445,            /* hostid_fromto  */
  YYSYMBOL_redirect = 446,                 /* redirect  */
  YYSYMBOL_sessionoption = 447,            /* sessionoption  */
  YYSYMBOL_sockssessionoption = 448,       /* sockssessionoption  */
  YYSYMBOL_crulesessionoption = 449,       /* crulesessionoption  */
  YYSYMBOL_sessioninheritable = 450,       /* sessioninheritable  */
  YYSYMBOL_sessionmax = 451,               /* sessionmax  */
  YYSYMBOL_sessionthrottle = 452,          /* sessionthrottle  */
  YYSYMBOL_sessionstate = 453,             /* sessionstate  */
  YYSYMBOL_sessionstate_key = 454,         /* sessionstate_key  */
  YYSYMBOL_sessionstate_keyinfo = 455,     /* sessionstate_keyinfo  */
  YYSYMBOL_456_24 = 456,                   /* $@24  */
  YYSYMBOL_sessionstate_max = 457,         /* sessionstate_max  */
  YYSYMBOL_sessionstate_throttle = 458,    /* sessionstate_throttle  */
  YYSYMBOL_bandwidth = 459,                /* bandwidth  */
  YYSYMBOL_log = 460,                      /* log  */
  YYSYMBOL_logname = 461,                  /* logname  */
  YYSYMBOL_logs = 462,                     /* logs  */
  YYSYMBOL_pamservicename = 463,           /* pamservicename  */
  YYSYMBOL_bsdauthstylename = 464,         /* bsdauthstylename  */
  YYSYMBOL_gssapiservicename = 465,        /* gssapiservicename  */
  YYSYMBOL_gssapikeytab = 466,             /* gssapikeytab  */
  YYSYMBOL_gssapienctype = 467,            /* gssapienctype  */
  YYSYMBOL_gssapienctypename = 468,        /* gssapienctypename  */
  YYSYMBOL_gssapienctypes = 469,           /* gssapienctypes  */
  YYSYMBOL_bounce = 470,                   /* bounce  */
  YYSYMBOL_libwrap = 471,                  /* libwrap  */
  YYSYMBOL_srcaddress = 472,               /* srcaddress  */
  YYSYMBOL_hostid_srcaddress = 473,        /* hostid_srcaddress  */
  YYSYMBOL_dstaddress = 474,               /* dstaddress  */
  YYSYMBOL_rdr_fromaddress = 475,          /* rdr_fromaddress  */
  YYSYMBOL_rdr_toaddress = 476,            /* rdr_toaddress  */
  YYSYMBOL_gateway = 477,                  /* gateway  */
  YYSYMBOL_routeoption = 478,              /* routeoption  */
  YYSYMBOL_routeoptions = 479,             /* routeoptions  */
  YYSYMBOL_routemethod = 480,              /* routemethod  */
  YYSYMBOL_from = 481,                     /* from  */
  YYSYMBOL_to = 482,                       /* to  */
  YYSYMBOL_rdr_from = 483,                 /* rdr_from  */
  YYSYMBOL_rdr_to = 484,                   /* rdr_to  */
  YYSYMBOL_bounceto = 485,                 /* bounceto  */
  YYSYMBOL_via = 486,                      /* via  */
  YYSYMBOL_externaladdress = 487,          /* externaladdress  */
  YYSYMBOL_address_without_port = 488,     /* address_without_port  */
  YYSYMBOL_address = 489,                  /* address  */
  YYSYMBOL_ipaddress = 490,                /* ipaddress  */
  YYSYMBOL_gwaddress = 491,                /* gwaddress  */
  YYSYMBOL_bouncetoaddress = 492,          /* bouncetoaddress  */
  YYSYMBOL_ipv4 = 493,                     /* ipv4  */
  YYSYMBOL_netmask_v4 = 494,               /* netmask_v4  */
  YYSYMBOL_ipv6 = 495,                     /* ipv6  */
  YYSYMBOL_netmask_v6 = 496,               /* netmask_v6  */
  YYSYMBOL_ipvany = 497,                   /* ipvany  */
  YYSYMBOL_netmask_vany = 498,             /* netmask_vany  */
  YYSYMBOL_domain = 499,                   /* domain  */
  YYSYMBOL_ifname = 500,                   /* ifname  */
  YYSYMBOL_url = 501,                      /* url  */
  YYSYMBOL_port = 502,                     /* port  */
  YYSYMBOL_gwport = 503,                   /* gwport  */
  YYSYMBOL_portnumber = 504,               /* portnumber  */
  YYSYMBOL_portrange = 505,                /* portrange  */
  YYSYMBOL_portstart = 506,                /* portstart  */
  YYSYMBOL_portend = 507,                  /* portend  */
  YYSYMBOL_portservice = 508,              /* portservice  */
  YYSYMBOL_portoperator = 509,             /* portoperator  */
  YYSYMBOL_udpportrange = 510,             /* udpportrange  */
  YYSYMBOL_udpportrange_start = 511,       /* udpportrange_start  */
  YYSYMBOL_udpportrange_end = 512,         /* udpportrange_end  */
  YYSYMBOL_number = 513,                   /* number  */
  YYSYMBOL_numbers = 514                   /* numbers  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  30
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   761

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  250
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  265
/* YYNRULES -- Number of rules.  */
#define YYNRULES  512
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  851

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   498


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,   249,   245,   248,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   244,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   246,     2,   247,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   655,   655,   655,   660,   663,   664,   667,   668,   669,
     670,   671,   674,   675,   677,   678,   679,   680,   681,   682,
     683,   684,   685,   686,   687,   688,   689,   690,   691,   692,
     693,   694,   695,   696,   697,   698,   699,   700,   701,   702,
     703,   704,   712,   713,   713,   718,   718,   726,   726,   736,
     736,   746,   746,   756,   756,   767,   777,   778,   781,   788,
     795,   802,   811,   812,   815,   867,   868,   869,   870,   873,
     880,   881,   880,   892,   893,   895,   898,   901,   904,   907,
     910,   913,   914,   917,   920,   928,   929,   932,   935,   943,
     944,   947,   950,   959,   960,   963,   964,   968,   972,   978,
    1003,  1027,  1027,  1052,  1059,  1081,  1081,  1094,  1098,  1101,
    1107,  1108,  1109,  1110,  1111,  1112,  1113,  1116,  1117,  1120,
    1128,  1138,  1155,  1155,  1158,  1158,  1161,  1218,  1219,  1222,
    1229,  1238,  1239,  1240,  1243,  1256,  1269,  1288,  1304,  1309,
    1312,  1318,  1325,  1330,  1338,  1358,  1359,  1362,  1376,  1390,
    1398,  1408,  1412,  1419,  1422,  1426,  1432,  1433,  1436,  1439,
    1442,  1449,  1454,  1455,  1458,  1500,  1561,  1561,  1568,  1580,
    1591,  1595,  1612,  1615,  1621,  1624,  1628,  1631,  1637,  1638,
    1641,  1653,  1653,  1664,  1664,  1679,  1682,  1683,  1686,  1695,
    1698,  1699,  1703,  1711,  1711,  1711,  1728,  1728,  1757,  1758,
    1759,  1762,  1766,  1769,  1775,  1779,  1782,  1788,  1788,  1850,
    1853,  1871,  1902,  1907,  1910,  1911,  1912,  1913,  1916,  1917,
    1920,  1925,  1930,  1931,  1936,  1939,  1939,  1964,  1965,  1968,
    1969,  1972,  1972,  1983,  1995,  1995,  2010,  2011,  2015,  2016,
    2017,  2018,  2019,  2020,  2021,  2022,  2027,  2031,  2037,  2038,
    2039,  2040,  2041,  2042,  2043,  2044,  2045,  2046,  2047,  2048,
    2049,  2054,  2059,  2067,  2072,  2095,  2096,  2099,  2100,  2101,
    2102,  2103,  2104,  2105,  2106,  2107,  2108,  2109,  2110,  2111,
    2112,  2113,  2114,  2115,  2118,  2119,  2120,  2121,  2122,  2123,
    2124,  2125,  2126,  2127,  2128,  2129,  2130,  2131,  2132,  2133,
    2134,  2135,  2136,  2137,  2138,  2139,  2140,  2141,  2142,  2143,
    2144,  2145,  2149,  2154,  2163,  2168,  2177,  2191,  2205,  2216,
    2230,  2244,  2258,  2274,  2286,  2300,  2312,  2324,  2336,  2348,
    2360,  2372,  2383,  2394,  2405,  2416,  2421,  2430,  2435,  2444,
    2449,  2458,  2463,  2472,  2477,  2486,  2491,  2500,  2505,  2515,
    2520,  2529,  2540,  2551,  2566,  2580,  2594,  2609,  2623,  2637,
    2651,  2663,  2677,  2691,  2703,  2715,  2730,  2745,  2763,  2781,
    2787,  2797,  2800,  2809,  2810,  2814,  2819,  2826,  2829,  2830,
    2833,  2836,  2839,  2845,  2849,  2855,  2858,  2859,  2862,  2865,
    2871,  2874,  2877,  2878,  2879,  2882,  2883,  2884,  2887,  2890,
    2891,  2894,  2898,  2904,  2913,  2922,  2923,  2924,  2925,  2928,
    2956,  2956,  2967,  2975,  2984,  2993,  2996,  3000,  3003,  3006,
    3009,  3012,  3018,  3019,  3023,  3035,  3048,  3060,  3076,  3079,
    3085,  3088,  3091,  3094,  3102,  3103,  3106,  3109,  3149,  3152,
    3155,  3158,  3161,  3171,  3174,  3175,  3176,  3177,  3178,  3179,
    3180,  3181,  3182,  3183,  3184,  3192,  3193,  3196,  3199,  3204,
    3209,  3214,  3219,  3227,  3232,  3233,  3234,  3235,  3238,  3239,
    3240,  3243,  3247,  3248,  3249,  3250,  3251,  3253,  3255,  3256,
    3257,  3258,  3261,  3262,  3266,  3274,  3281,  3287,  3295,  3304,
    3312,  3322,  3328,  3335,  3342,  3343,  3344,  3345,  3348,  3349,
    3352,  3353,  3356,  3364,  3370,  3376,  3411,  3417,  3420,  3427,
    3439,  3444,  3445
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ALARM",
  "ALARMTYPE_DATA", "ALARMTYPE_DISCONNECT", "ALARMIF_INTERNAL",
  "ALARMIF_EXTERNAL", "TCPOPTION_DISABLED", "ECN", "SACK", "TIMESTAMPS",
  "WSCALE", "MTU_ERROR", "CLIENTCOMPATIBILITY", "NECGSSAPI", "CLIENTRULE",
  "HOSTIDRULE", "SOCKSRULE", "COMPATIBILITY", "SAMEPORT", "DRAFT_5_05",
  "CONNECTTIMEOUT", "TCP_FIN_WAIT", "CPU", "MASK", "SCHEDULE",
  "CPUMASK_ANYCPU", "DEBUGGING", "DEPRECATED", "ERRORLOG", "LOGOUTPUT",
  "LOGFILE", "LOGTYPE_ERROR", "LOGTYPE_TCP_DISABLED",
  "LOGTYPE_TCP_ENABLED", "LOGIF_INTERNAL", "LOGIF_EXTERNAL", "ERRORVALUE",
  "EXTENSION", "BIND", "PRIVILEGED", "EXTERNAL_PROTOCOL",
  "INTERNAL_PROTOCOL", "EXTERNAL_ROTATION", "SAMESAME", "GROUPNAME",
  "HOSTID", "HOSTINDEX", "INTERFACE", "SOCKETOPTION_SYMBOLICVALUE",
  "INTERNAL", "EXTERNAL", "INTERNALSOCKET", "EXTERNALSOCKET", "IOTIMEOUT",
  "IOTIMEOUT_TCP", "IOTIMEOUT_UDP", "NEGOTIATETIMEOUT", "LIBWRAP_FILE",
  "LOGLEVEL", "SOCKSMETHOD", "CLIENTMETHOD", "METHOD", "METHODNAME",
  "NONE", "BSDAUTH", "GSSAPI", "PAM_ADDRESS", "PAM_ANY", "PAM_USERNAME",
  "RFC931", "UNAME", "MONITOR", "PROCESSTYPE", "PROC_MAXREQUESTS",
  "PROC_MAXLIFETIME", "REALM", "REALNAME", "RESOLVEPROTOCOL", "REQUIRED",
  "SCHEDULEPOLICY", "SERVERCONFIG", "CLIENTCONFIG", "SOCKET",
  "CLIENTSIDE_SOCKET", "SNDBUF", "RCVBUF", "SOCKETPROTOCOL",
  "SOCKETOPTION_OPTID", "SRCHOST", "NODNSMISMATCH", "NODNSUNKNOWN",
  "CHECKREPLYAUTH", "USERNAME", "USER_PRIVILEGED", "USER_UNPRIVILEGED",
  "USER_LIBWRAP", "WORD__IN", "ROUTE", "VIA", "GLOBALROUTEOPTION",
  "BADROUTE_EXPIRE", "MAXFAIL", "PORT", "NUMBER", "BANDWIDTH", "BOUNCE",
  "BSDAUTHSTYLE", "BSDAUTHSTYLENAME", "COMMAND", "COMMAND_BIND",
  "COMMAND_CONNECT", "COMMAND_UDPASSOCIATE", "COMMAND_BINDREPLY",
  "COMMAND_UDPREPLY", "ACTION", "FROM", "TO", "GSSAPIENCTYPE",
  "GSSAPIENC_ANY", "GSSAPIENC_CLEAR", "GSSAPIENC_INTEGRITY",
  "GSSAPIENC_CONFIDENTIALITY", "GSSAPIENC_PERMESSAGE", "GSSAPIKEYTAB",
  "GSSAPISERVICE", "GSSAPISERVICENAME", "GSSAPIKEYTABNAME", "IPV4", "IPV6",
  "IPVANY", "DOMAINNAME", "IFNAME", "URL", "LDAPATTRIBUTE",
  "LDAPATTRIBUTE_AD", "LDAPATTRIBUTE_HEX", "LDAPATTRIBUTE_AD_HEX",
  "LDAPBASEDN", "LDAP_BASEDN", "LDAPBASEDN_HEX", "LDAPBASEDN_HEX_ALL",
  "LDAPCERTFILE", "LDAPCERTPATH", "LDAPPORT", "LDAPPORTSSL", "LDAPDEBUG",
  "LDAPDEPTH", "LDAPAUTO", "LDAPSEARCHTIME", "LDAPDOMAIN", "LDAP_DOMAIN",
  "LDAPFILTER", "LDAPFILTER_AD", "LDAPFILTER_HEX", "LDAPFILTER_AD_HEX",
  "LDAPGROUP", "LDAPGROUP_NAME", "LDAPGROUP_HEX", "LDAPGROUP_HEX_ALL",
  "LDAPKEYTAB", "LDAPKEYTABNAME", "LDAPDEADTIME", "LDAPSERVER",
  "LDAPSERVER_NAME", "LDAPAUTHSERVER", "LDAPAUTHKEYTAB", "LDAPSSL",
  "LDAPCERTCHECK", "LDAPKEEPREALM", "LDAPTIMEOUT", "LDAPCACHE",
  "LDAPCACHEPOS", "LDAPCACHENEG", "LDAPURL", "LDAP_URL", "LDAPAUTHBASEDN",
  "LDAPAUTHBASEDN_HEX", "LDAPAUTHBASEDN_HEX_ALL", "LDAPAUTHURL",
  "LDAPAUTHPORT", "LDAPAUTHPORTSSL", "LDAPAUTHDEBUG", "LDAPAUTHSSL",
  "LDAPAUTHAUTO", "LDAPAUTHCERTCHECK", "LDAPAUTHFILTER", "LDAPAUTHDOMAIN",
  "LDAPAUTHCERTFILE", "LDAPAUTHCERTPATH", "LDAPAUTHKEEPREALM",
  "LDAP_FILTER", "LDAP_ATTRIBUTE", "LDAP_CERTFILE", "LDAP_CERTPATH",
  "LIBWRAPSTART", "LIBWRAP_ALLOW", "LIBWRAP_DENY", "LIBWRAP_HOSTS_ACCESS",
  "LINE", "OPERATOR", "PACSID", "PACSID_B64", "PACSID_FLAG", "PACSID_NAME",
  "PAMSERVICENAME", "PROTOCOL", "PROTOCOL_TCP", "PROTOCOL_UDP",
  "PROTOCOL_FAKE", "PROXYPROTOCOL", "PROXYPROTOCOL_SOCKS_V4",
  "PROXYPROTOCOL_SOCKS_V5", "PROXYPROTOCOL_HTTP", "PROXYPROTOCOL_UPNP",
  "REDIRECT", "SENDSIDE", "RECVSIDE", "SERVICENAME", "SESSION_INHERITABLE",
  "SESSIONMAX", "SESSIONTHROTTLE", "SESSIONSTATE_KEY", "SESSIONSTATE_MAX",
  "SESSIONSTATE_THROTTLE", "RULE_LOG", "RULE_LOG_CONNECT", "RULE_LOG_DATA",
  "RULE_LOG_DISCONNECT", "RULE_LOG_ERROR", "RULE_LOG_IOOPERATION",
  "RULE_LOG_TCPINFO", "STATEKEY", "UDPPORTRANGE", "UDPCONNECTDST", "USER",
  "GROUP", "VERDICT_BLOCK", "VERDICT_PASS", "YES", "NO", "LOGFORMAT",
  "LOGFORMAT_VALUE", "':'", "'.'", "'{'", "'}'", "'/'", "'-'", "$accept",
  "configtype", "$@1", "serverobjects", "serverobject", "serveroptions",
  "serveroption", "logspecial", "$@2", "$@3", "internal_if_logoption",
  "$@4", "external_if_logoption", "$@5", "rule_internal_logoption", "$@6",
  "rule_external_logoption", "$@7", "loglevel", "tcpoptions", "tcpoption",
  "errors", "errorobject", "timeout", "deprecated", "route", "$@8", "$@9",
  "routes", "proxyprotocol", "proxyprotocolname", "proxyprotocols", "user",
  "username", "usernames", "group", "groupname", "groupnames", "extension",
  "extensionname", "extensions", "ifprotocols", "ifprotocol", "internal",
  "internalinit", "internal_protocol", "$@10", "external", "externalinit",
  "external_protocol", "$@11", "external_rotation", "clientoption",
  "clientoptions", "global_routeoption", "logformat", "errorlog", "$@12",
  "logoutput", "$@13", "logoutputdevice", "logoutputdevices", "childstate",
  "userids", "user_privileged", "user_unprivileged", "user_libwrap",
  "userid", "iotimeout", "negotiatetimeout", "connecttimeout",
  "tcp_fin_timeout", "debugging", "libwrapfiles", "libwrap_allowfile",
  "libwrap_denyfile", "libwrap_hosts_access", "udpconnectdst",
  "compatibility", "compatibilityname", "compatibilitynames",
  "resolveprotocol", "resolveprotocolname", "cpu", "cpuschedule",
  "cpuaffinity", "socketoption", "$@14", "socketoptionname",
  "socketoptionvalue", "socketside", "srchost", "srchostoption",
  "srchostoptions", "realm", "global_clientmethod", "$@15",
  "global_socksmethod", "$@16", "socksmethod", "socksmethods",
  "socksmethodname", "clientmethod", "clientmethods", "clientmethodname",
  "monitor", "$@17", "$@18", "crule", "$@19", "alarm", "monitorside",
  "alarmside", "alarm_data", "$@20", "alarm_test", "networkproblem",
  "alarm_disconnect", "alarmperiod", "monitoroption", "monitoroptions",
  "cruleoption", "hrule", "$@21", "cruleoptions", "hostidoption", "hostid",
  "$@22", "hostindex", "srule", "$@23", "sruleoptions", "sruleoption",
  "genericruleoption", "ldapauthoption", "ldapoption", "ldapdebug",
  "ldapauthdebug", "ldapdomain", "ldapauthdomain", "ldapdepth",
  "ldapcertfile", "ldapauthcertfile", "ldapcertpath", "ldapauthcertpath",
  "ldapurl", "ldapauthurl", "ldapauthbasedn", "ldapauthbasedn_hex",
  "ldapauthbasedn_hex_all", "lbasedn", "lbasedn_hex", "lbasedn_hex_all",
  "ldapauthport", "ldapport", "ldapauthportssl", "ldapportssl", "ldapssl",
  "ldapauthssl", "ldapauto", "ldapauthauto", "ldapcertcheck",
  "ldapauthcertcheck", "ldapauthkeeprealm", "ldapkeeprealm", "ldapfilter",
  "ldapauthfilter", "ldapfilter_ad", "ldapfilter_hex", "ldapfilter_ad_hex",
  "ldapattribute", "ldapattribute_ad", "ldapattribute_hex",
  "ldapattribute_ad_hex", "lgroup_hex", "lgroup_hex_all", "lgroup",
  "lserver", "ldapauthserver", "ldapkeytab", "ldapauthkeytab", "psid",
  "psid_b64", "psid_off", "clientcompatibility", "clientcompatibilityname",
  "clientcompatibilitynames", "verdict", "command", "commands",
  "commandname", "protocol", "protocols", "protocolname", "fromto",
  "hostid_fromto", "redirect", "sessionoption", "sockssessionoption",
  "crulesessionoption", "sessioninheritable", "sessionmax",
  "sessionthrottle", "sessionstate", "sessionstate_key",
  "sessionstate_keyinfo", "$@24", "sessionstate_max",
  "sessionstate_throttle", "bandwidth", "log", "logname", "logs",
  "pamservicename", "bsdauthstylename", "gssapiservicename",
  "gssapikeytab", "gssapienctype", "gssapienctypename", "gssapienctypes",
  "bounce", "libwrap", "srcaddress", "hostid_srcaddress", "dstaddress",
  "rdr_fromaddress", "rdr_toaddress", "gateway", "routeoption",
  "routeoptions", "routemethod", "from", "to", "rdr_from", "rdr_to",
  "bounceto", "via", "externaladdress", "address_without_port", "address",
  "ipaddress", "gwaddress", "bouncetoaddress", "ipv4", "netmask_v4",
  "ipv6", "netmask_v6", "ipvany", "netmask_vany", "domain", "ifname",
  "url", "port", "gwport", "portnumber", "portrange", "portstart",
  "portend", "portservice", "portoperator", "udpportrange",
  "udpportrange_start", "udpportrange_end", "number", "numbers", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-762)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-219)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      66,  -762,   228,   118,    55,  -195,  -185,  -174,  -762,  -172,
    -109,  -105,  -102,  -100,   -69,   -44,    86,  -762,  -762,   228,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,   -14,  -125,  -762,  -762,    11,    20,    34,    37,  -762,
    -762,  -762,  -762,    45,    88,    94,   107,   110,   111,   119,
     122,   123,   124,   125,   128,   129,   139,  -762,    55,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
     174,  -762,  -762,  -762,  -762,   191,   209,   213,  -762,  -762,
     215,   221,   257,   259,    29,   140,   141,  -762,   266,   134,
     138,   126,   136,   342,  -762,  -762,     3,   142,   143,  -762,
    -762,   283,   284,   314,   113,   302,   302,   302,   338,   341,
     -27,   -23,   158,   120,  -762,   157,  -762,  -762,  -762,   371,
     371,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,   299,
     300,  -762,  -762,  -762,  -762,   134,  -762,   161,   162,   348,
     348,  -762,   342,  -762,    92,    92,  -762,  -762,  -762,    28,
     137,   345,   346,  -762,  -762,  -762,  -762,  -762,  -762,   113,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,   371,  -762,  -762,  -762,  -762,   165,
    -762,   339,   340,  -762,   167,   170,  -762,  -762,  -762,  -762,
      92,  -762,  -762,  -762,  -762,  -762,  -762,   312,  -762,  -762,
     169,   171,   172,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,   345,  -762,  -762,   346,  -762,    49,    49,    49,
     176,   -47,  -762,  -762,   177,   179,   214,   214,  -762,   -72,
    -762,   -68,   313,   319,  -762,  -762,  -762,  -762,   180,   181,
     182,  -762,  -762,  -762,   185,    61,   320,   349,   188,   189,
     190,  -762,  -762,  -762,  -762,   -73,  -762,   186,   -73,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,   154,   154,   524,    75,
     -10,   192,   193,   194,   195,   196,   197,   198,   199,   327,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,    61,
     328,  -762,  -762,   320,  -762,   183,   408,  -762,  -762,  -762,
    -762,  -762,  -762,   343,  -762,  -762,  -762,   203,   205,   206,
     207,   208,   335,   210,   211,   212,   216,   217,   175,   218,
     219,   222,    59,   223,   224,   225,   226,   227,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,   154,   328,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,   328,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,   256,   258,   260,   261,   262,   263,
     264,   265,   267,   268,   269,   270,   272,   273,   274,   275,
     277,   280,   281,   282,  -762,   328,   524,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,   271,  -762,  -762,  -762,    16,   328,
    -762,  -762,  -762,  -762,  -762,  -762,   442,   345,   187,    74,
     330,   332,    97,    14,  -762,  -762,   285,  -762,  -762,   364,
     347,   286,  -762,   367,  -762,  -762,   408,    35,    35,  -762,
    -762,   287,   288,  -762,   396,   345,   346,   398,  -762,   290,
     310,   322,   323,    68,   316,  -762,   413,  -762,   292,    70,
     432,   433,   306,  -762,   435,   436,     4,   448,   497,  -762,
     154,   154,   347,   301,   439,   351,   356,   357,   358,   412,
     414,   415,   359,   361,   452,   453,   -79,   457,    72,   411,
     372,   373,   374,   375,   410,   418,   425,   407,   405,   419,
     426,    76,    82,    84,   397,   447,   449,   450,   416,   486,
     488,   -76,    87,    90,    93,   402,   443,   403,   401,    95,
     493,   524,  -762,   354,  -762,   360,  -762,    18,  -762,   442,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,   187,  -762,
    -762,  -762,  -762,  -762,    74,  -762,  -762,  -762,  -762,  -762,
    -762,    97,  -762,  -762,  -762,  -762,  -762,    14,  -762,    28,
    -762,    61,   362,  -762,  -762,   363,    28,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,    35,  -762,   348,   348,    28,  -762,
    -762,  -762,  -762,    52,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,    28,  -762,  -762,  -762,   352,  -762,   553,  -762,   355,
    -762,  -762,  -762,  -762,  -762,  -762,     4,  -762,  -762,  -762,
     448,  -762,  -762,   497,   365,   366,  -762,    28,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,   500,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,   503,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,   353,   368,   596,   127,   505,
     369,  -762,  -762,  -762,  -762,  -762,  -762,   370,   112,    28,
    -762,  -762,   377,   378,  -762,   507,  -762,   507,  -762,   509,
    -762,   513,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
     514,  -762,  -762,  -762,  -762,  -762,   380,   379,  -762,  -762,
    -762,   507,  -762,   507,  -762,  -762,  -762,   214,   214,   424,
    -762,  -762,  -762,  -762,  -762,  -762,   521,   523,  -762,  -762,
    -762,  -762,   -73,   531,   533,  -762,   528,   530,  -762,  -762,
    -762
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       0,     2,   117,     0,    12,     0,     0,     0,    69,     0,
       0,     0,     0,     0,     0,     0,     0,   116,   111,   117,
      73,   112,   113,   114,    66,    67,    65,    68,   110,   115,
       1,     0,     0,    47,    49,     0,     0,     0,     0,   100,
     104,   172,   173,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     5,    12,    30,
      24,    38,    18,    20,    28,    29,    21,    22,    23,    27,
      34,    19,    33,    14,    40,   131,   132,   133,    17,    32,
     145,   146,    31,    39,    15,    36,    16,   162,   163,    41,
       0,    37,    35,    25,    26,     0,     0,     0,   122,   124,
       0,     0,     0,     0,     0,     0,     0,   118,     4,     0,
       0,     0,     0,     0,   105,   101,     0,     0,     0,   183,
     181,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     3,    13,     0,   142,   143,   144,     0,
       0,   138,   139,   140,   141,   160,   161,   159,   158,     0,
       0,    70,    74,   154,   155,   156,   153,     0,     0,     0,
       0,    92,    93,    91,     0,     0,   108,   107,   109,     0,
       0,     0,     0,   129,   130,   180,   175,   176,   177,   178,
     174,   137,   134,   135,   136,   147,   148,   149,   150,   151,
     152,   121,   196,   225,   234,   193,     6,    11,    10,     7,
       8,     9,   166,   126,   127,   123,   125,   120,   119,     0,
     157,     0,     0,    55,     0,     0,    94,    97,    98,   106,
      95,   102,   484,   487,   489,   491,   492,   494,    99,   468,
     473,   475,   477,   469,   470,   103,   464,   465,   466,   467,
     188,   184,   186,   192,   182,   190,   179,     0,     0,     0,
       0,     0,   128,    71,     0,     0,     0,     0,    96,     0,
     471,     0,     0,     0,   187,   191,   375,   376,     0,     0,
       0,   194,   169,   168,     0,   455,     0,     0,     0,     0,
       0,    48,    50,   503,   506,     0,   497,     0,     0,   485,
     486,   472,   488,   474,   490,   476,   227,   227,   236,   201,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     452,   447,   454,   446,   445,   448,   449,   450,   451,   455,
       0,   444,   510,   511,   165,     0,     0,    43,    45,   505,
     495,   501,   500,     0,   496,    51,    53,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   256,   250,
     265,   266,   251,   264,   249,   248,   227,     0,   255,   229,
     230,   224,   260,   261,   262,   222,   221,   263,   400,   223,
     399,   395,   396,   397,   405,   406,   408,   407,   247,   258,
     259,   254,   253,   252,   220,   257,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   244,     0,   236,   240,   242,   241,
     295,   274,   297,   273,   296,   293,   281,   294,   282,   311,
     269,   270,   271,   272,   289,   290,   291,   275,   304,   276,
     305,   306,   277,   288,   278,   292,   280,   283,   302,   298,
     279,   299,   301,   300,   284,   285,   287,   286,   308,   309,
     307,   310,   267,   303,   268,   239,   243,   398,   245,   238,
     246,   202,   203,   214,     0,   198,   200,   199,   201,     0,
     216,   215,   217,   171,   170,   167,     0,     0,     0,     0,
       0,     0,     0,     0,   460,   453,     0,   456,   458,     0,
       0,     0,   512,     0,    64,    42,    62,     0,     0,   504,
     502,     0,     0,   231,     0,     0,     0,     0,   462,     0,
       0,     0,     0,     0,     0,   461,   393,   394,     0,     0,
       0,     0,     0,   410,     0,     0,     0,     0,     0,   228,
     227,   227,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   236,   237,     0,   207,     0,   219,   201,   372,   373,
     371,   457,   380,   381,   382,   383,   384,   377,   378,   429,
     430,   431,   432,   433,   434,   428,   427,   426,   388,   389,
     385,   386,    76,    77,    78,    79,    80,    81,    75,     0,
     463,   455,     0,   459,   390,     0,     0,   164,    63,    58,
      59,    60,    61,    44,    56,    46,     0,     0,     0,   233,
     185,   189,   414,     0,   437,   367,   368,   369,   370,   424,
     392,     0,   401,   402,   403,     0,   409,     0,   412,     0,
     416,   417,   418,   419,   420,   421,   422,   415,    84,    85,
      83,    88,    89,    87,     0,     0,   391,     0,   425,   356,
     357,   358,   359,   328,   329,   330,   319,   321,   332,   334,
     312,     0,   318,   339,   340,   316,   351,   353,   354,   355,
     362,   360,   361,   365,   363,   364,   366,   335,   336,   343,
     344,   349,   350,   323,   325,   326,   327,   324,   331,   333,
     314,     0,   337,   338,   341,   342,   345,   346,   352,   317,
     320,   322,   347,   348,   508,     0,     0,     0,   204,     0,
       0,   374,   379,   435,   387,    82,   441,     0,     0,     0,
     438,    57,     0,     0,   232,   498,   436,   498,   442,     0,
     411,     0,   423,    86,    90,   197,   226,   439,   313,   315,
       0,   235,   210,   209,   206,   205,     0,     0,   195,    72,
     493,   498,   443,   498,   480,   481,   440,     0,     0,     0,
     482,   483,   404,   413,   509,   507,     0,     0,   478,   479,
      52,    54,     0,     0,   212,   499,     0,     0,   211,   208,
     213
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -762,  -762,  -762,  -762,  -762,   578,  -762,  -253,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -154,  -500,
    -762,   101,  -762,    69,    -3,   506,  -762,  -762,  -762,  -281,
    -762,   -19,  -762,   -70,  -762,  -762,   -71,  -762,    46,  -762,
     479,  -124,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,   625,    63,  -762,    83,  -762,    85,  -762,
    -762,  -101,  -762,  -762,  -762,  -762,  -762,   220,  -762,  -762,
    -762,  -762,    89,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
     490,   121,  -762,  -762,  -762,  -762,    -4,  -762,  -762,  -762,
    -762,  -762,  -762,   467,  -762,  -762,  -762,  -762,  -762,  -762,
    -226,  -762,  -762,  -225,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -474,  -762,  -762,  -762,  -276,  -288,  -762,  -762,   -50,  -762,
    -762,  -418,  -762,  -280,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -267,  -762,    19,   100,  -284,    13,
    -762,  -268,     1,  -762,  -340,  -762,  -762,  -279,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,   -53,  -762,  -762,  -266,  -263,  -262,  -762,    12,
    -762,  -762,  -762,  -762,   102,   307,   108,  -762,  -762,  -309,
    -762,   291,  -762,  -762,  -762,  -762,  -762,  -762,  -598,  -590,
    -648,  -762,  -762,   512,  -762,   516,  -762,  -762,  -762,  -168,
    -167,  -762,  -762,  -761,  -283,  -762,   399,  -762,  -762,  -153,
    -762,  -762,  -762,  -762,   393
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     3,     4,   133,   196,    57,    58,   281,   537,   538,
      59,   111,    60,   112,   358,   541,   359,   542,   214,   673,
     674,   535,   536,   360,    18,   152,   209,   275,   108,   310,
     657,   658,   361,   709,   710,   362,   712,   713,   311,   162,
     163,   219,   220,    64,   117,    65,   165,    66,   118,    67,
     164,    68,    19,    20,    21,    70,    22,   139,    23,   140,
     204,   205,    73,    74,    75,    76,    77,   182,    24,    25,
      26,    27,    28,    79,    80,    81,    82,    83,    84,   155,
     156,    29,   148,    86,    87,    88,   363,   251,   274,   515,
      90,    91,   179,   180,    92,    93,   172,    94,   171,   364,
     241,   242,   365,   244,   245,   198,   250,   299,   199,   247,
     503,   504,   816,   505,   778,   506,   813,   507,   848,   508,
     509,   366,   200,   248,   367,   368,   369,   678,   370,   201,
     249,   445,   446,   371,   448,   449,   450,   451,   452,   453,
     454,   455,   456,   457,   458,   459,   460,   461,   462,   463,
     464,   465,   466,   467,   468,   469,   470,   471,   472,   473,
     474,   475,   476,   477,   478,   479,   480,   481,   482,   483,
     484,   485,   486,   487,   488,   489,   490,   491,   492,   493,
     494,   372,   373,   374,   375,   629,   630,   268,   314,   637,
     638,   376,   650,   651,   529,   571,   377,   378,   498,   379,
     380,   381,   382,   383,   384,   385,   697,   386,   387,   388,
     389,   706,   707,   390,   499,   391,   392,   393,   644,   645,
     394,   395,   530,   572,   664,   525,   557,   661,   319,   320,
     321,   531,   665,   526,   558,   549,   662,   235,   227,   228,
     229,   822,   796,   230,   291,   231,   293,   232,   295,   233,
     234,   825,   260,   830,   330,   286,   331,   540,   332,   288,
     500,   775,   835,   323,   324
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      89,    62,   238,   239,   282,   334,   215,   315,   313,   316,
     527,   510,   317,   318,   495,   511,   264,   444,   447,   497,
     265,   396,   501,   502,   501,   502,   730,   570,   622,   760,
     496,   512,   283,   283,   626,   795,   831,   289,   675,   206,
     513,   221,   272,     8,   669,   670,   671,   672,   166,    95,
      63,   315,   313,   316,    89,    62,   317,   318,   273,    96,
     838,   290,   839,   337,   338,   337,   338,    69,   167,   786,
      97,    17,    98,    61,    31,   301,   790,     5,     6,    32,
     794,   501,   502,     7,     8,     9,    10,    71,    17,    72,
     569,    33,    34,    78,    35,   514,   258,    36,    37,    38,
      35,   798,   168,   252,    63,   621,    39,    40,    41,    42,
      11,    12,    13,    14,    41,    42,    43,    44,    30,   807,
     110,    69,   337,   338,   302,    85,   303,    61,   303,   284,
      45,    46,    47,  -218,    15,    99,   192,   193,   194,   100,
     821,    71,   101,    72,   102,    48,   329,    78,     1,     2,
      49,    50,    51,   780,   153,   154,    16,   222,   223,   224,
     225,   226,   495,   157,   158,   444,   447,   497,   301,   627,
     731,   303,   285,   761,   791,   103,     5,     6,   496,    85,
     304,   222,   223,   224,   225,   303,   305,   306,   105,   106,
     335,   336,  -218,   195,   639,   640,   641,   642,   643,   826,
     104,   337,   338,   776,   176,   177,   178,    41,    42,    11,
      12,    13,    14,   187,   188,   339,   340,   189,   190,   151,
     510,   217,   218,   307,   511,   307,   652,   653,   654,   655,
     109,   700,   701,   702,   703,   704,   705,   145,   146,   147,
     512,   222,   223,   224,   225,   226,   820,   278,   279,   280,
       5,     6,    52,    53,    54,   113,     7,     8,     9,    10,
     341,   342,   135,  -218,   114,  -218,   222,   223,   307,   225,
     226,   312,   308,   304,   623,   624,   625,   309,   115,   305,
     306,   116,   307,    11,    12,    13,    14,   266,   267,   119,
      55,   631,   524,   555,   714,   715,   136,    56,   632,   633,
     634,   635,   636,   562,   563,   648,   649,    15,   687,   688,
     692,   693,   733,   734,   137,   312,   747,   748,   138,   680,
     141,   681,   749,   750,   751,   752,   142,   762,   763,    16,
     764,   765,   120,   766,   767,   772,   773,   495,   121,   510,
     444,   447,   497,   511,   814,   815,   183,   184,   269,   270,
     343,   122,   787,   496,   123,   124,   344,   345,   346,   512,
     347,   307,   143,   125,   144,   151,   126,   127,   128,   129,
     348,   159,   130,   131,   349,   350,   351,   352,   353,   354,
     355,   160,   161,   132,   149,   150,   169,   170,   173,   174,
     356,   357,   175,   315,   313,   316,   181,   185,   317,   318,
     186,   191,   202,   203,   207,   208,   211,   212,   213,   240,
     243,   253,   256,   254,   255,   257,   259,   261,   292,   262,
     263,   276,   271,   277,   294,   322,   296,   297,   298,   300,
     325,   533,   326,   327,   328,   333,   516,   517,   518,   519,
     520,   521,   522,   523,   524,   528,   534,   543,   539,   544,
     545,   546,   547,   548,   550,   551,   552,   628,   646,   647,
     553,   554,   559,   560,   660,   663,   561,   564,   565,   566,
     567,   568,   667,   574,   575,   576,   577,   578,   579,   580,
     581,   582,   583,   584,   585,   586,   587,   588,   589,   590,
     591,   592,   593,   594,   595,   596,   597,   598,   599,   600,
     601,   679,   602,   682,   603,   604,   605,   606,   607,   608,
     684,   609,   610,   611,   612,   797,   613,   614,   615,   616,
     656,   617,   792,   793,   618,   619,   620,   685,   686,   659,
     666,   555,   676,   677,   683,   689,   691,   694,   695,   696,
     698,   699,   708,   711,   719,   717,     5,     6,   718,   720,
     721,   722,   723,   726,   724,   725,   727,   728,   729,   845,
     335,   336,   732,   735,   736,   737,   738,   739,   740,   743,
     744,   337,   338,   753,   840,   841,   741,    41,    42,    11,
      12,    13,    14,   742,   745,   339,   340,   754,   746,   755,
     756,   758,   757,   759,   768,   769,   771,   770,   774,   777,
     799,   338,   810,   801,   779,   808,   788,   789,   809,   812,
     817,   829,   805,   806,   832,   811,   818,   819,   833,   834,
     823,   824,   827,   828,   836,   284,   843,   837,   844,   846,
     341,   847,   397,   849,   303,   850,   134,   668,   785,   197,
     803,   216,   804,   304,   107,   210,   246,   800,   781,   305,
     306,   782,   784,   802,   656,   556,   783,   312,   287,   398,
     399,   400,   401,   402,   690,   403,   404,   405,   406,   407,
     408,   409,   410,   411,   716,   412,   842,   413,   414,   415,
     416,   417,   236,   418,   419,   420,   237,   573,   421,     0,
     422,   423,   424,   425,   426,     0,     0,     0,     0,   427,
       0,   428,   429,   430,   431,   432,   433,   434,   435,   436,
     437,   438,   439,   440,   441,   442,   532,     0,     0,     0,
     343,     0,     0,     0,     0,     0,   344,   345,   346,     0,
     347,   307,     0,     0,     0,   308,     0,     0,     0,     0,
     348,     0,     0,     0,     0,   350,   351,   352,   353,   354,
     355,     0,     0,     0,     0,     0,     0,     0,   443,     0,
     356,   357
};

static const yytype_int16 yycheck[] =
{
       4,     4,   170,   170,   257,   288,   160,   275,   275,   275,
     319,   299,   275,   275,   298,   299,   242,   298,   298,   298,
     245,   297,     6,     7,     6,     7,   105,   367,   446,   105,
     298,   299,   105,   105,   508,   683,   797,   105,   538,   140,
      50,   165,    89,    29,     9,    10,    11,    12,    45,   244,
       4,   319,   319,   319,    58,    58,   319,   319,   105,   244,
     821,   129,   823,    47,    48,    47,    48,     4,    65,   659,
     244,     2,   244,     4,    19,    14,   666,    22,    23,    24,
     678,     6,     7,    28,    29,    30,    31,     4,    19,     4,
     366,    36,    37,     4,    39,   105,   220,    42,    43,    44,
      39,   691,    99,   204,    58,   445,    51,    52,    53,    54,
      55,    56,    57,    58,    53,    54,    61,    62,     0,   717,
     245,    58,    47,    48,    63,     4,   110,    58,   110,   201,
      75,    76,    77,   117,    79,   244,    16,    17,    18,   244,
     788,    58,   244,    58,   244,    90,   219,    58,    82,    83,
      95,    96,    97,   627,    20,    21,   101,   129,   130,   131,
     132,   133,   446,    25,    26,   446,   446,   446,    14,   509,
     249,   110,   244,   249,   674,   244,    22,    23,   446,    58,
     119,   129,   130,   131,   132,   110,   125,   126,   102,   103,
      36,    37,   117,    73,   120,   121,   122,   123,   124,   789,
     244,    47,    48,   621,    91,    92,    93,    53,    54,    55,
      56,    57,    58,   240,   241,    61,    62,   240,   241,    99,
     508,   129,   130,   207,   508,   207,   212,   213,   214,   215,
     244,   227,   228,   229,   230,   231,   232,   208,   209,   210,
     508,   129,   130,   131,   132,   133,   134,    33,    34,    35,
      22,    23,   197,   198,   199,   244,    28,    29,    30,    31,
     106,   107,    88,   247,   244,   247,   129,   130,   207,   132,
     133,   275,   211,   119,     3,     4,     5,   216,   244,   125,
     126,   244,   207,    55,    56,    57,    58,   238,   239,   244,
     235,   517,   117,   118,   570,   571,   105,   242,   111,   112,
     113,   114,   115,   244,   245,   208,   209,    79,   240,   241,
     240,   241,   240,   241,   105,   319,   240,   241,   105,   545,
     105,   546,   240,   241,   240,   241,   105,   240,   241,   101,
     240,   241,   244,   240,   241,   240,   241,   621,   244,   627,
     621,   621,   621,   627,   217,   218,   126,   127,   248,   249,
     196,   244,   661,   621,   244,   244,   202,   203,   204,   627,
     206,   207,   105,   244,   105,    99,   244,   244,   244,   244,
     216,   245,   244,   244,   220,   221,   222,   223,   224,   225,
     226,   245,    40,   244,   244,   244,   244,   244,   105,   105,
     236,   237,    78,   661,   661,   661,    94,    59,   661,   661,
      59,   243,   245,    32,   105,   105,   245,   245,    60,    64,
      64,   246,   245,    74,    74,   245,   104,   248,   105,   248,
     248,   244,   246,   244,   105,   105,   246,   246,   246,   244,
      81,   248,   244,   244,   244,   249,   244,   244,   244,   244,
     244,   244,   244,   244,   117,   117,    38,   244,   105,   244,
     244,   244,   244,   118,   244,   244,   244,    15,   128,   127,
     244,   244,   244,   244,   100,   118,   244,   244,   244,   244,
     244,   244,   105,   244,   244,   244,   244,   244,   244,   244,
     244,   244,   244,   244,   244,   244,   244,   244,   244,   244,
     244,   244,   244,   244,   244,   244,   244,   244,   244,   244,
     244,   105,   244,   105,   244,   244,   244,   244,   244,   244,
     200,   244,   244,   244,   244,   683,   244,   244,   244,   244,
     523,   244,   676,   677,   244,   244,   244,   205,   205,   244,
     244,   118,   245,   245,   244,   219,   244,   105,   105,   233,
     105,   105,    94,    46,   193,   244,    22,    23,   109,   193,
     193,   193,   140,   194,   140,   140,   195,   105,   105,   842,
      36,    37,   105,   152,   192,   192,   192,   192,   158,   162,
     165,    47,    48,   176,   827,   828,   158,    53,    54,    55,
      56,    57,    58,   158,   165,    61,    62,   140,   162,   140,
     140,   105,   176,   105,   192,   152,   195,   194,   105,   245,
     248,    48,   249,   248,   244,   105,   244,   244,   105,    13,
     105,   104,   247,   247,   105,   247,   247,   247,   105,   105,
     788,   788,   245,   245,   244,   201,   105,   248,   105,    98,
     106,    98,   108,   105,   110,   105,    58,   536,   657,   133,
     710,   162,   713,   119,    19,   155,   179,   697,   629,   125,
     126,   638,   651,   706,   657,   348,   644,   661,   259,   135,
     136,   137,   138,   139,   556,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   572,   151,   829,   153,   154,   155,
     156,   157,   170,   159,   160,   161,   170,   396,   164,    -1,
     166,   167,   168,   169,   170,    -1,    -1,    -1,    -1,   175,
      -1,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   323,    -1,    -1,    -1,
     196,    -1,    -1,    -1,    -1,    -1,   202,   203,   204,    -1,
     206,   207,    -1,    -1,    -1,   211,    -1,    -1,    -1,    -1,
     216,    -1,    -1,    -1,    -1,   221,   222,   223,   224,   225,
     226,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   234,    -1,
     236,   237
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,    82,    83,   251,   252,    22,    23,    28,    29,    30,
      31,    55,    56,    57,    58,    79,   101,   273,   274,   302,
     303,   304,   306,   308,   318,   319,   320,   321,   322,   331,
       0,    19,    24,    36,    37,    39,    42,    43,    44,    51,
      52,    53,    54,    61,    62,    75,    76,    77,    90,    95,
      96,    97,   197,   198,   199,   235,   242,   255,   256,   260,
     262,   273,   274,   288,   293,   295,   297,   299,   301,   304,
     305,   306,   308,   312,   313,   314,   315,   316,   322,   323,
     324,   325,   326,   327,   328,   331,   333,   334,   335,   336,
     340,   341,   344,   345,   347,   244,   244,   244,   244,   244,
     244,   244,   244,   244,   244,   102,   103,   303,   278,   244,
     245,   261,   263,   244,   244,   244,   244,   294,   298,   244,
     244,   244,   244,   244,   244,   244,   244,   244,   244,   244,
     244,   244,   244,   253,   255,    88,   105,   105,   105,   307,
     309,   105,   105,   105,   105,   208,   209,   210,   332,   244,
     244,    99,   275,    20,    21,   329,   330,    25,    26,   245,
     245,    40,   289,   290,   300,   296,    45,    65,    99,   244,
     244,   348,   346,   105,   105,    78,    91,    92,    93,   342,
     343,    94,   317,   317,   317,    59,    59,   240,   241,   240,
     241,   243,    16,    17,    18,    73,   254,   275,   355,   358,
     372,   379,   245,    32,   310,   311,   311,   105,   105,   276,
     330,   245,   245,    60,   268,   268,   290,   129,   130,   291,
     292,   291,   129,   130,   131,   132,   133,   488,   489,   490,
     493,   495,   497,   499,   500,   487,   493,   495,   499,   500,
      64,   350,   351,    64,   353,   354,   343,   359,   373,   380,
     356,   337,   311,   246,    74,    74,   245,   245,   291,   104,
     502,   248,   248,   248,   350,   353,   238,   239,   437,   437,
     437,   246,    89,   105,   338,   277,   244,   244,    33,    34,
      35,   257,   257,   105,   201,   244,   505,   506,   509,   105,
     129,   494,   105,   496,   105,   498,   246,   246,   246,   357,
     244,    14,    63,   110,   119,   125,   126,   207,   211,   216,
     279,   288,   336,   434,   438,   441,   465,   466,   467,   478,
     479,   480,   105,   513,   514,    81,   244,   244,   244,   219,
     504,   506,   508,   249,   504,    36,    37,    47,    48,    61,
      62,   106,   107,   196,   202,   203,   204,   206,   216,   220,
     221,   222,   223,   224,   225,   226,   236,   237,   264,   266,
     273,   282,   285,   336,   349,   352,   371,   374,   375,   376,
     378,   383,   431,   432,   433,   434,   441,   446,   447,   449,
     450,   451,   452,   453,   454,   455,   457,   458,   459,   460,
     463,   465,   466,   467,   470,   471,   374,   108,   135,   136,
     137,   138,   139,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   151,   153,   154,   155,   156,   157,   159,   160,
     161,   164,   166,   167,   168,   169,   170,   175,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   234,   279,   381,   382,   383,   384,   385,
     386,   387,   388,   389,   390,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   414,   415,
     416,   417,   418,   419,   420,   421,   422,   423,   424,   425,
     426,   427,   428,   429,   430,   438,   441,   447,   448,   464,
     510,     6,     7,   360,   361,   363,   365,   367,   369,   370,
     375,   438,   441,    50,   105,   339,   244,   244,   244,   244,
     244,   244,   244,   244,   117,   475,   483,   479,   117,   444,
     472,   481,   514,   248,    38,   271,   272,   258,   259,   105,
     507,   265,   267,   244,   244,   244,   244,   244,   118,   485,
     244,   244,   244,   244,   244,   118,   475,   476,   484,   244,
     244,   244,   244,   245,   244,   244,   244,   244,   244,   374,
     444,   445,   473,   481,   244,   244,   244,   244,   244,   244,
     244,   244,   244,   244,   244,   244,   244,   244,   244,   244,
     244,   244,   244,   244,   244,   244,   244,   244,   244,   244,
     244,   244,   244,   244,   244,   244,   244,   244,   244,   244,
     244,   244,   244,   244,   244,   244,   244,   244,   244,   244,
     244,   444,   381,     3,     4,     5,   370,   444,    15,   435,
     436,   350,   111,   112,   113,   114,   115,   439,   440,   120,
     121,   122,   123,   124,   468,   469,   128,   127,   208,   209,
     442,   443,   212,   213,   214,   215,   274,   280,   281,   244,
     100,   477,   486,   118,   474,   482,   244,   105,   271,     9,
      10,    11,    12,   269,   270,   269,   245,   245,   377,   105,
     350,   353,   105,   244,   200,   205,   205,   240,   241,   219,
     476,   244,   240,   241,   105,   105,   233,   456,   105,   105,
     227,   228,   229,   230,   231,   232,   461,   462,    94,   283,
     284,    46,   286,   287,   374,   374,   474,   244,   109,   193,
     193,   193,   193,   140,   140,   140,   194,   195,   105,   105,
     105,   249,   105,   240,   241,   152,   192,   192,   192,   192,
     158,   158,   158,   162,   165,   165,   162,   240,   241,   240,
     241,   240,   241,   176,   140,   140,   140,   176,   105,   105,
     105,   249,   240,   241,   240,   241,   240,   241,   192,   152,
     194,   195,   240,   241,   105,   511,   381,   245,   364,   244,
     370,   436,   439,   469,   442,   281,   489,   479,   244,   244,
     489,   269,   268,   268,   488,   490,   492,   499,   489,   248,
     378,   248,   462,   283,   286,   247,   247,   488,   105,   105,
     249,   247,    13,   366,   217,   218,   362,   105,   247,   247,
     134,   490,   491,   499,   500,   501,   489,   245,   245,   104,
     503,   503,   105,   105,   105,   512,   244,   248,   503,   503,
     257,   257,   509,   105,   105,   504,    98,    98,   368,   105,
     105
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   250,   252,   251,   251,   253,   253,   254,   254,   254,
     254,   254,   255,   255,   256,   256,   256,   256,   256,   256,
     256,   256,   256,   256,   256,   256,   256,   256,   256,   256,
     256,   256,   256,   256,   256,   256,   256,   256,   256,   256,
     256,   256,   257,   258,   257,   259,   257,   261,   260,   263,
     262,   265,   264,   267,   266,   268,   269,   269,   270,   270,
     270,   270,   271,   271,   272,   273,   273,   273,   273,   274,
     276,   277,   275,   278,   278,   279,   280,   280,   280,   280,
     280,   281,   281,   282,   283,   284,   284,   285,   286,   287,
     287,   288,   289,   290,   290,   291,   291,   292,   292,   293,
     294,   296,   295,   297,   298,   300,   299,   301,   301,   301,
     302,   302,   302,   302,   302,   302,   302,   303,   303,   304,
     304,   305,   307,   306,   309,   308,   310,   311,   311,   312,
     312,   313,   313,   313,   314,   315,   316,   317,   318,   318,
     318,   319,   320,   321,   322,   323,   323,   324,   325,   326,
     326,   327,   327,   328,   329,   329,   330,   330,   331,   332,
     332,   332,   333,   333,   334,   335,   337,   336,   338,   338,
     339,   339,   340,   340,   341,   342,   342,   342,   343,   343,
     344,   346,   345,   348,   347,   349,   350,   350,   351,   352,
     353,   353,   354,   356,   357,   355,   359,   358,   360,   360,
     360,   361,   361,   361,   362,   362,   362,   364,   363,   365,
     366,   367,   368,   368,   369,   369,   369,   369,   370,   370,
     371,   371,   371,   371,   371,   373,   372,   374,   374,   375,
     375,   377,   376,   378,   380,   379,   381,   381,   382,   382,
     382,   382,   382,   382,   382,   382,   382,   383,   383,   383,
     383,   383,   383,   383,   383,   383,   383,   383,   383,   383,
     383,   383,   383,   383,   383,   383,   383,   384,   384,   384,
     384,   384,   384,   384,   384,   384,   384,   384,   384,   384,
     384,   384,   384,   384,   385,   385,   385,   385,   385,   385,
     385,   385,   385,   385,   385,   385,   385,   385,   385,   385,
     385,   385,   385,   385,   385,   385,   385,   385,   385,   385,
     385,   385,   386,   386,   387,   387,   388,   389,   390,   391,
     392,   393,   394,   395,   396,   397,   398,   399,   400,   401,
     402,   403,   404,   405,   406,   407,   407,   408,   408,   409,
     409,   410,   410,   411,   411,   412,   412,   413,   413,   414,
     414,   415,   416,   417,   418,   419,   420,   421,   422,   423,
     424,   425,   426,   427,   428,   429,   430,   431,   432,   433,
     433,   434,   435,   436,   436,   437,   437,   438,   439,   439,
     440,   440,   440,   440,   440,   441,   442,   442,   443,   443,
     444,   445,   446,   446,   446,   447,   447,   447,   448,   449,
     449,   450,   450,   451,   452,   453,   453,   453,   453,   454,
     456,   455,   457,   458,   459,   460,   461,   461,   461,   461,
     461,   461,   462,   462,   463,   464,   465,   466,   467,   468,
     468,   468,   468,   468,   469,   469,   470,   471,   472,   473,
     474,   475,   476,   477,   478,   478,   478,   478,   478,   478,
     478,   478,   478,   478,   478,   479,   479,   480,   481,   482,
     483,   484,   485,   486,   487,   487,   487,   487,   488,   488,
     488,   489,   490,   490,   490,   490,   490,   490,   491,   491,
     491,   491,   492,   492,   493,   494,   494,   495,   496,   497,
     498,   499,   500,   501,   502,   502,   502,   502,   503,   503,
     504,   504,   505,   506,   507,   508,   509,   510,   511,   512,
     513,   514,   514
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     4,     3,     0,     2,     1,     1,     1,
       1,     1,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     0,     4,     0,     4,     0,     6,     0,
       6,     0,     6,     0,     6,     1,     1,     2,     1,     1,
       1,     1,     1,     2,     1,     1,     1,     1,     1,     1,
       0,     0,     9,     0,     2,     3,     1,     1,     1,     1,
       1,     1,     2,     3,     1,     1,     2,     3,     1,     1,
       2,     3,     1,     1,     2,     1,     2,     1,     1,     4,
       0,     0,     4,     4,     0,     0,     4,     3,     3,     3,
       1,     1,     1,     1,     1,     1,     1,     0,     2,     4,
       4,     3,     0,     4,     0,     4,     1,     1,     2,     3,
       3,     1,     1,     1,     3,     3,     3,     1,     3,     3,
       3,     3,     3,     3,     3,     1,     1,     3,     3,     3,
       3,     3,     3,     3,     1,     1,     1,     2,     3,     1,
       1,     1,     1,     1,     9,     7,     0,     7,     1,     1,
       1,     1,     1,     1,     3,     1,     1,     1,     1,     2,
       3,     0,     4,     0,     4,     3,     1,     2,     1,     3,
       1,     2,     1,     0,     0,     8,     0,     8,     1,     1,
       1,     0,     1,     1,     0,     1,     1,     0,     8,     4,
       1,     7,     0,     2,     1,     1,     1,     1,     0,     2,
       1,     1,     1,     1,     1,     0,     8,     0,     2,     1,
       1,     0,     4,     3,     0,     8,     0,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     4,     3,     4,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     1,     1,     2,     1,     1,     3,     1,     2,
       1,     1,     1,     1,     1,     3,     1,     2,     1,     1,
       2,     2,     3,     2,     2,     1,     1,     1,     1,     1,
       1,     3,     3,     3,     5,     1,     1,     1,     1,     3,
       0,     4,     3,     5,     3,     3,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     3,     3,     3,     3,     1,
       1,     1,     1,     1,     1,     2,     4,     3,     3,     3,
       3,     3,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     1,     0,     2,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     3,     1,     3,     1,     3,     1,     2,     2,
       1,     1,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     3,     3,     2,     0,     3,
       1,     1,     3,     1,     1,     1,     1,     5,     1,     1,
       1,     1,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* $@1: %empty  */
#line 655 "config_parse.y"
                           {
#if !SOCKS_CLIENT
      extension = &sockscf.extension;
#endif /* !SOCKS_CLIENT*/
   }
#line 3243 "config_parse.c"
    break;

  case 5: /* serverobjects: %empty  */
#line 663 "config_parse.y"
               { (yyval.string) = NULL; }
#line 3249 "config_parse.c"
    break;

  case 12: /* serveroptions: %empty  */
#line 674 "config_parse.y"
                { (yyval.string) = NULL; }
#line 3255 "config_parse.c"
    break;

  case 41: /* serveroption: socketoption  */
#line 704 "config_parse.y"
                            {
      if (!addedsocketoption(&sockscf.socketoptionc,
                             &sockscf.socketoptionv,
                             &socketopt))
         yywarn("could not add socket option");
   }
#line 3266 "config_parse.c"
    break;

  case 43: /* $@2: %empty  */
#line 713 "config_parse.y"
                                     {
#if !SOCKS_CLIENT
                                tcpoptions = &logspecial->protocol.tcp.disabled;
#endif /* !SOCKS_CLIENT */
          }
#line 3276 "config_parse.c"
    break;

  case 45: /* $@3: %empty  */
#line 718 "config_parse.y"
                                    {
#if !SOCKS_CLIENT
                                tcpoptions = &logspecial->protocol.tcp.enabled;
#endif /* !SOCKS_CLIENT */
          }
#line 3286 "config_parse.c"
    break;

  case 47: /* $@4: %empty  */
#line 726 "config_parse.y"
                                      {
#if !SOCKS_CLIENT

      logspecial = &sockscf.internal.log;

#endif /* !SOCKS_CLIENT */

   }
#line 3299 "config_parse.c"
    break;

  case 49: /* $@5: %empty  */
#line 736 "config_parse.y"
                                      {
#if !SOCKS_CLIENT

      logspecial = &sockscf.external.log;

#endif /* !SOCKS_CLIENT */

   }
#line 3312 "config_parse.c"
    break;

  case 51: /* $@6: %empty  */
#line 746 "config_parse.y"
                                        {
#if !SOCKS_CLIENT

      logspecial = &rule.internal.log;

#endif /* !SOCKS_CLIENT */

   }
#line 3325 "config_parse.c"
    break;

  case 53: /* $@7: %empty  */
#line 756 "config_parse.y"
                                        {
#if !SOCKS_CLIENT

      logspecial = &rule.external.log;

#endif /* !SOCKS_CLIENT */

   }
#line 3338 "config_parse.c"
    break;

  case 55: /* loglevel: LOGLEVEL  */
#line 767 "config_parse.y"
                   {
#if !SOCKS_CLIENT
   SASSERTX((yyvsp[0].number) >= 0);
   SASSERTX((yyvsp[0].number) < MAXLOGLEVELS);

   cloglevel = (yyvsp[0].number);
#endif /* !SOCKS_CLIENT */
   }
#line 3351 "config_parse.c"
    break;

  case 58: /* tcpoption: ECN  */
#line 781 "config_parse.y"
               {
#if !SOCKS_CLIENT
   SET_TCPOPTION(tcpoptions, cloglevel, ecn);
#endif /* !SOCKS_CLIENT */
   }
#line 3361 "config_parse.c"
    break;

  case 59: /* tcpoption: SACK  */
#line 788 "config_parse.y"
                {
#if !SOCKS_CLIENT
   SET_TCPOPTION(tcpoptions, cloglevel, sack);
#endif /* !SOCKS_CLIENT */
   }
#line 3371 "config_parse.c"
    break;

  case 60: /* tcpoption: TIMESTAMPS  */
#line 795 "config_parse.y"
                      {
#if !SOCKS_CLIENT
   SET_TCPOPTION(tcpoptions, cloglevel, timestamps);
#endif /* !SOCKS_CLIENT */
   }
#line 3381 "config_parse.c"
    break;

  case 61: /* tcpoption: WSCALE  */
#line 802 "config_parse.y"
                  {
#if !SOCKS_CLIENT
   SET_TCPOPTION(tcpoptions, cloglevel, wscale);
#endif /* !SOCKS_CLIENT */
   }
#line 3391 "config_parse.c"
    break;

  case 64: /* errorobject: ERRORVALUE  */
#line 815 "config_parse.y"
                        {
#if !SOCKS_CLIENT

   if ((yyvsp[0].error).valuev == NULL)
      yywarnx("unknown error symbol specified");
   else {
      size_t *ec, ec_max, i;
      int *ev;

      switch ((yyvsp[0].error).valuetype) {
         case VALUETYPE_ERRNO:
            ev     = logspecial->errno_loglevelv[cloglevel];
            ec     = &logspecial->errno_loglevelc[cloglevel];
            ec_max = ELEMENTS(logspecial->errno_loglevelv[cloglevel]);
            break;

         case VALUETYPE_GAIERR:
            ev     = logspecial->gaierr_loglevelv[cloglevel];
            ec     = &logspecial->gaierr_loglevelc[cloglevel];
            ec_max = ELEMENTS(logspecial->gaierr_loglevelv[cloglevel]);
            break;

         default:
            SERRX((yyvsp[0].error).valuetype);
      }

      for (i = 0; (yyvsp[0].error).valuev[i] != 0; ++i) {
         /*
          * If the value is already set in the array, e.g. because some
          * errno-symbols have the same values, ignore this value.
          */
         size_t j;

         for (j = 0; j < *ec; ++j) {
            if (ev[j] == (yyvsp[0].error).valuev[i])
               break;
         }

         if (j < *ec)
            continue; /* error-value already set in array. */

         SASSERTX(*ec < ec_max);

         ev[(*ec)] = (yyvsp[0].error).valuev[i];
         ++(*ec);
      }
   }
#endif /* !SOCKS_CLIENT */
   }
#line 3445 "config_parse.c"
    break;

  case 69: /* deprecated: DEPRECATED  */
#line 873 "config_parse.y"
                         {
      yyerrorx("given keyword \"%s\" is deprecated.  New keyword is %s.  "
               "Please see %s's manual for more information",
               (yyvsp[0].deprecated).oldname, (yyvsp[0].deprecated).newname, PRODUCT);
   }
#line 3455 "config_parse.c"
    break;

  case 70: /* $@8: %empty  */
#line 880 "config_parse.y"
               { objecttype = object_route; }
#line 3461 "config_parse.c"
    break;

  case 71: /* $@9: %empty  */
#line 881 "config_parse.y"
         { routeinit(&route); }
#line 3467 "config_parse.c"
    break;

  case 72: /* route: ROUTE $@8 '{' $@9 routeoptions fromto gateway routeoptions '}'  */
#line 881 "config_parse.y"
                                                                             {
      route.src       = src;
      route.dst       = dst;
      route.gw.addr   = gw;

      route.rdr_from  = rdr_from;

      socks_addroute(&route, 1);
   }
#line 3481 "config_parse.c"
    break;

  case 73: /* routes: %empty  */
#line 892 "config_parse.y"
        { (yyval.string) = NULL; }
#line 3487 "config_parse.c"
    break;

  case 76: /* proxyprotocolname: PROXYPROTOCOL_SOCKS_V4  */
#line 898 "config_parse.y"
                                            {
         state->proxyprotocol.socks_v4 = 1;
   }
#line 3495 "config_parse.c"
    break;

  case 77: /* proxyprotocolname: PROXYPROTOCOL_SOCKS_V5  */
#line 901 "config_parse.y"
                              {
         state->proxyprotocol.socks_v5 = 1;
   }
#line 3503 "config_parse.c"
    break;

  case 78: /* proxyprotocolname: PROXYPROTOCOL_HTTP  */
#line 904 "config_parse.y"
                         {
         state->proxyprotocol.http     = 1;
   }
#line 3511 "config_parse.c"
    break;

  case 79: /* proxyprotocolname: PROXYPROTOCOL_UPNP  */
#line 907 "config_parse.y"
                         {
         state->proxyprotocol.upnp     = 1;
   }
#line 3519 "config_parse.c"
    break;

  case 84: /* username: USERNAME  */
#line 920 "config_parse.y"
                     {
#if !SOCKS_CLIENT
      if (addlinkedname(&rule.user, (yyvsp[0].string)) == NULL)
         yyerror(NOMEM);
#endif /* !SOCKS_CLIENT */
   }
#line 3530 "config_parse.c"
    break;

  case 88: /* groupname: GROUPNAME  */
#line 935 "config_parse.y"
                       {
#if !SOCKS_CLIENT
      if (addlinkedname(&rule.group, (yyvsp[0].string)) == NULL)
         yyerror(NOMEM);
#endif /* !SOCKS_CLIENT */
   }
#line 3541 "config_parse.c"
    break;

  case 92: /* extensionname: BIND  */
#line 950 "config_parse.y"
                      {
         yywarnx("we are currently considering deprecating the Dante-specific "
                 "SOCKS bind extension.  If you are using it, please let us "
                 "know on the public dante-misc@inet.no mailinglist");

         extension->bind = 1;
   }
#line 3553 "config_parse.c"
    break;

  case 97: /* ifprotocol: IPV4  */
#line 968 "config_parse.y"
                 {
#if !SOCKS_CLIENT
      ifproto->ipv4  = 1;
   }
#line 3562 "config_parse.c"
    break;

  case 98: /* ifprotocol: IPV6  */
#line 972 "config_parse.y"
           {
      ifproto->ipv6  = 1;
#endif /* SOCKS_SERVER */
   }
#line 3571 "config_parse.c"
    break;

  case 99: /* internal: INTERNAL internalinit ':' address  */
#line 978 "config_parse.y"
                                              {
#if !SOCKS_CLIENT
#if BAREFOOTD
      yyerrorx("\"internal:\" specification is not used in %s", PRODUCT);
#endif /* BAREFOOTD */

      interfaceprotocol_t ifprotozero;

      bzero(&ifprotozero, sizeof(ifprotozero));
      if (memcmp(&ifprotozero,
                 &sockscf.internal.protocol,
                 sizeof(sockscf.internal.protocol)) == 0) {
         slog(LOG_DEBUG, "%s: no address families explicitly enabled on "
                         "internal interface.  Enabling default address "
                         "families",
                         function);

         sockscf.internal.protocol.ipv4 = sockscf.internal.protocol.ipv6 = 1;
      }

      addinternal(ruleaddr, SOCKS_TCP);
#endif /* !SOCKS_CLIENT */
   }
#line 3599 "config_parse.c"
    break;

  case 100: /* internalinit: %empty  */
#line 1003 "config_parse.y"
              {
#if !SOCKS_CLIENT
   static ruleaddr_t mem;
   struct servent    *service;
   serverstate_t     statemem;

   bzero(&statemem, sizeof(statemem));
   state               = &statemem;
   state->protocol.tcp = 1;

   bzero(&logspecial, sizeof(logspecial));

   bzero(&mem, sizeof(mem));
   addrinit(&mem, 0);

   /* set default port. */
   if ((service = getservbyname("socks", "tcp")) == NULL)
      *port_tcp = htons(SOCKD_PORT);
   else
      *port_tcp = (in_port_t)service->s_port;
#endif /* !SOCKS_CLIENT */
   }
#line 3626 "config_parse.c"
    break;

  case 101: /* $@10: %empty  */
#line 1027 "config_parse.y"
                                         {
#if !SOCKS_CLIENT
      if (sockscf.internal.addrc > 0) {
         if (sockscf.state.inited) {
            /*
             * Must be running due to SIGHUP.  The internal interface requires
             * special considerations, so let the SIGHUP code deal with this
             * later when we know if the change in protocol also results in.
             * adding a new interface.
             */
            ;
         }
         else {
            log_interfaceprotocol_set_too_late(INTERNALIF);
            exit(1);
         }
      }

      ifproto = &sockscf.internal.protocol;
#endif /* !SOCKS_CLIENT */
   }
#line 3652 "config_parse.c"
    break;

  case 103: /* external: EXTERNAL externalinit ':' externaladdress  */
#line 1052 "config_parse.y"
                                                      {
#if !SOCKS_CLIENT
      addexternal(ruleaddr);
#endif /* !SOCKS_CLIENT */
   }
#line 3662 "config_parse.c"
    break;

  case 104: /* externalinit: %empty  */
#line 1059 "config_parse.y"
              {
#if !SOCKS_CLIENT
      static ruleaddr_t mem;
      interfaceprotocol_t ifprotozero = { 0 };

      bzero(&mem, sizeof(mem));
      addrinit(&mem, 0);

      if (memcmp(&ifprotozero,
                 &sockscf.external.protocol,
                 sizeof(sockscf.external.protocol)) == 0) {
         slog(LOG_DEBUG, "%s: no address families explicitly enabled on "
                         "external interface.  Enabling default address "
                         "families",
                         function);

         sockscf.external.protocol.ipv4 = sockscf.external.protocol.ipv6 = 1;
      }
#endif /* !SOCKS_CLIENT */
   }
#line 3687 "config_parse.c"
    break;

  case 105: /* $@11: %empty  */
#line 1081 "config_parse.y"
                                         {
#if !SOCKS_CLIENT
      if (sockscf.external.addrc > 0) {
         log_interfaceprotocol_set_too_late(EXTERNALIF);
         sockdexit(EXIT_FAILURE);
      }

      ifproto = &sockscf.external.protocol;
#endif /* !SOCKS_CLIENT */
   }
#line 3702 "config_parse.c"
    break;

  case 107: /* external_rotation: EXTERNAL_ROTATION ':' NONE  */
#line 1094 "config_parse.y"
                                                {
#if !SOCKS_CLIENT
      sockscf.external.rotation = ROTATION_NONE;
   }
#line 3711 "config_parse.c"
    break;

  case 108: /* external_rotation: EXTERNAL_ROTATION ':' SAMESAME  */
#line 1098 "config_parse.y"
                                      {
      sockscf.external.rotation = ROTATION_SAMESAME;
   }
#line 3719 "config_parse.c"
    break;

  case 109: /* external_rotation: EXTERNAL_ROTATION ':' ROUTE  */
#line 1101 "config_parse.y"
                                   {
      sockscf.external.rotation = ROTATION_ROUTE;
#endif /* SOCKS_SERVER */
   }
#line 3728 "config_parse.c"
    break;

  case 117: /* clientoptions: %empty  */
#line 1116 "config_parse.y"
               { (yyval.string) = NULL; }
#line 3734 "config_parse.c"
    break;

  case 119: /* global_routeoption: GLOBALROUTEOPTION MAXFAIL ':' NUMBER  */
#line 1120 "config_parse.y"
                                                         {
      if ((yyvsp[0].number) < 0)
         yyerrorx("max route fails can not be negative (%ld)  Use \"0\" to "
                  "indicate routes should never be marked as bad",
                  (long)(yyvsp[0].number));

      sockscf.routeoptions.maxfail = (yyvsp[0].number);
   }
#line 3747 "config_parse.c"
    break;

  case 120: /* global_routeoption: GLOBALROUTEOPTION BADROUTE_EXPIRE ':' NUMBER  */
#line 1128 "config_parse.y"
                                                   {
      if ((yyvsp[0].number) < 0)
         yyerrorx("route failure expiry time can not be negative (%ld).  "
                  "Use \"0\" to indicate bad route marking should never expire",
                  (long)(yyvsp[0].number));

      sockscf.routeoptions.badexpire = (yyvsp[0].number);
   }
#line 3760 "config_parse.c"
    break;

  case 121: /* logformat: LOGFORMAT ':' LOGFORMAT_VALUE  */
#line 1138 "config_parse.y"
                                         {
#if !SOCKS_CLIENT
      if (logformat_seen)
         yyerrorx("duplicate logformat directive");

      if (strcmp((yyvsp[0].string), "raw") == 0)
         sockscf.logformat = LOGFORMAT_RAW;
      else if (strcmp((yyvsp[0].string), "json") == 0)
         sockscf.logformat = LOGFORMAT_JSON;
      else
         yyerrorx("invalid logformat \"%s\": expected raw or json", (yyvsp[0].string));

      logformat_seen = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 3780 "config_parse.c"
    break;

  case 122: /* $@12: %empty  */
#line 1155 "config_parse.y"
                         { add_to_errlog = 1; }
#line 3786 "config_parse.c"
    break;

  case 124: /* $@13: %empty  */
#line 1158 "config_parse.y"
                         { add_to_errlog = 0; }
#line 3792 "config_parse.c"
    break;

  case 126: /* logoutputdevice: LOGFILE  */
#line 1161 "config_parse.y"
                         {
   int p;

   if ((add_to_errlog && failed_to_add_errlog)
   ||      (!add_to_errlog && failed_to_add_log)) {
      yywarnx("not adding logfile \"%s\"", (yyvsp[0].string));

      slog(LOG_ALERT,
           "%s: not trying to add logfile \"%s\" due to having already failed "
           "adding logfiles during this SIGHUP.  Only if all logfiles "
           "specified in the config can be added will we switch to using "
           "the new logfiles.  Until then, we will continue using only the "
           "old logfiles",
           function, (yyvsp[0].string));
   }
   else {
      p = socks_addlogfile(add_to_errlog ? &sockscf.errlog : &sockscf.log, (yyvsp[0].string));

#if !SOCKS_CLIENT
      if (sockscf.state.inited) {
         if (p == -1) {
            if (add_to_errlog) {
               sockscf.errlog       = old_errlog;
               failed_to_add_errlog = 1;
            }
            else {
               sockscf.log          = old_log;
               failed_to_add_log    = 1;
            }
         }
         else {
            sockd_freelogobject(add_to_errlog ?  &old_errlog : &old_log, 1);
            slog(LOG_DEBUG, "%s: added logfile \"%s\" to %s",
                 function, (yyvsp[0].string), add_to_errlog ? "errlog" : "logoutput");
         }
      }

      if (p == -1)
         slog(LOG_ALERT, "%s: could not (re)open logfile \"%s\": %s%s  %s",
              function,
              (yyvsp[0].string),
              strerror(errno),
              sockscf.state.inited ?
                  "." : "",
              sockscf.state.inited ?
                  "Will continue using old logfiles" : "");

#else /* SOCKS_CLIENT  */
      if (p == -1)
         /*
          * bad, but don't consider it fatal in the client.
          */
         yywarn("failed to add logfile %s", (yyvsp[0].string));
#endif /* SOCKS_CLIENT */
   }
}
#line 3853 "config_parse.c"
    break;

  case 129: /* childstate: PROC_MAXREQUESTS ':' NUMBER  */
#line 1222 "config_parse.y"
                                        {
#if !SOCKS_CLIENT

      ASSIGN_NUMBER((yyvsp[0].number), >=, 0, sockscf.child.maxrequests, 0);

#endif /* !SOCKS_CLIENT */
   }
#line 3865 "config_parse.c"
    break;

  case 130: /* childstate: PROC_MAXLIFETIME ':' NUMBER  */
#line 1229 "config_parse.y"
                                 {
#if !SOCKS_CLIENT

      ASSIGN_NUMBER((yyvsp[0].number), >=, 0, sockscf.child.maxlifetime, 0);

#endif /* !SOCKS_CLIENT */
   }
#line 3877 "config_parse.c"
    break;

  case 134: /* user_privileged: USER_PRIVILEGED ':' userid  */
#line 1243 "config_parse.y"
                                              {
#if !SOCKS_CLIENT
#if HAVE_PRIVILEGES
      yyerrorx("userid-settings not used on platforms with privileges");
#else
      sockscf.uid.privileged_uid   = (yyvsp[0].uid).uid;
      sockscf.uid.privileged_gid   = (yyvsp[0].uid).gid;
      sockscf.uid.privileged_isset = 1;
#endif /* !HAVE_PRIVILEGES */
#endif /* !SOCKS_CLIENT */
   }
#line 3893 "config_parse.c"
    break;

  case 135: /* user_unprivileged: USER_UNPRIVILEGED ':' userid  */
#line 1256 "config_parse.y"
                                                  {
#if !SOCKS_CLIENT
#if HAVE_PRIVILEGES
      yyerrorx("userid-settings not used on platforms with privileges");
#else
      sockscf.uid.unprivileged_uid   = (yyvsp[0].uid).uid;
      sockscf.uid.unprivileged_gid   = (yyvsp[0].uid).gid;
      sockscf.uid.unprivileged_isset = 1;
#endif /* !HAVE_PRIVILEGES */
#endif /* !SOCKS_CLIENT */
   }
#line 3909 "config_parse.c"
    break;

  case 136: /* user_libwrap: USER_LIBWRAP ':' userid  */
#line 1269 "config_parse.y"
                                        {
#if HAVE_LIBWRAP && (!SOCKS_CLIENT)

#if HAVE_PRIVILEGES
      yyerrorx("userid-settings not used on platforms with privileges");

#else
      sockscf.uid.libwrap_uid   = (yyvsp[0].uid).uid;
      sockscf.uid.libwrap_gid   = (yyvsp[0].uid).gid;
      sockscf.uid.libwrap_isset = 1;
#endif /* !HAVE_PRIVILEGES */

#else  /* !HAVE_LIBWRAP && (!SOCKS_CLIENT) */
      yyerrorx_nolib("libwrap");
#endif /* !HAVE_LIBWRAP (!SOCKS_CLIENT)*/
   }
#line 3930 "config_parse.c"
    break;

  case 137: /* userid: USERNAME  */
#line 1288 "config_parse.y"
                   {
      struct passwd *pw;

      if ((pw = getpwnam((yyvsp[0].string))) == NULL)
         yyerror("getpwnam(3) says no such user \"%s\"", (yyvsp[0].string));

      (yyval.uid).uid = pw->pw_uid;

      if ((pw = getpwuid((yyval.uid).uid)) == NULL)
         yyerror("getpwuid(3) says no such uid %lu (from user \"%s\")",
                 (unsigned long)(yyval.uid).uid, (yyvsp[0].string));

      (yyval.uid).gid = pw->pw_gid;
   }
#line 3949 "config_parse.c"
    break;

  case 138: /* iotimeout: IOTIMEOUT ':' NUMBER  */
#line 1304 "config_parse.y"
                                  {
#if !SOCKS_CLIENT
      ASSIGN_NUMBER((yyvsp[0].number), >=, 0, timeout->tcpio, 1);
      timeout->udpio = timeout->tcpio;
   }
#line 3959 "config_parse.c"
    break;

  case 139: /* iotimeout: IOTIMEOUT_TCP ':' NUMBER  */
#line 1309 "config_parse.y"
                               {
      ASSIGN_NUMBER((yyvsp[0].number), >=, 0, timeout->tcpio, 1);
   }
#line 3967 "config_parse.c"
    break;

  case 140: /* iotimeout: IOTIMEOUT_UDP ':' NUMBER  */
#line 1312 "config_parse.y"
                               {
      ASSIGN_NUMBER((yyvsp[0].number), >=, 0, timeout->udpio, 1);
#endif /* !SOCKS_CLIENT */
   }
#line 3976 "config_parse.c"
    break;

  case 141: /* negotiatetimeout: NEGOTIATETIMEOUT ':' NUMBER  */
#line 1318 "config_parse.y"
                                                {
#if !SOCKS_CLIENT
      ASSIGN_NUMBER((yyvsp[0].number), >=, 0, timeout->negotiate, 1);
#endif /* !SOCKS_CLIENT */
   }
#line 3986 "config_parse.c"
    break;

  case 142: /* connecttimeout: CONNECTTIMEOUT ':' NUMBER  */
#line 1325 "config_parse.y"
                                            {
      ASSIGN_NUMBER((yyvsp[0].number), >=, 0, timeout->connect, 1);
   }
#line 3994 "config_parse.c"
    break;

  case 143: /* tcp_fin_timeout: TCP_FIN_WAIT ':' NUMBER  */
#line 1330 "config_parse.y"
                                           {
#if !SOCKS_CLIENT
      ASSIGN_NUMBER((yyvsp[0].number), >=, 0, timeout->tcp_fin_wait, 1);
#endif /* !SOCKS_CLIENT */
   }
#line 4004 "config_parse.c"
    break;

  case 144: /* debugging: DEBUGGING ':' NUMBER  */
#line 1338 "config_parse.y"
                                {
#if SOCKS_CLIENT

       sockscf.option.debug = (int)(yyvsp[0].number);

#else /* !SOCKS_CLIENT */

      if (sockscf.initial.cmdline.debug_isset
      &&  sockscf.initial.cmdline.debug != (yyvsp[0].number))
         LOG_CMDLINE_OVERRIDE("debug",
                              sockscf.initial.cmdline.debug,
                              (int)(yyvsp[0].number),
                              "%d");
      else
         sockscf.option.debug = (int)(yyvsp[0].number);

#endif /* !SOCKS_CLIENT */
   }
#line 4027 "config_parse.c"
    break;

  case 147: /* libwrap_allowfile: LIBWRAP_ALLOW ':' LIBWRAP_FILE  */
#line 1362 "config_parse.y"
                                                  {
#if !SOCKS_CLIENT
#if HAVE_LIBWRAP
      if ((hosts_allow_table  = strdup((yyvsp[0].string))) == NULL)
         yyerror(NOMEM);

      slog(LOG_DEBUG, "%s: libwrap.allow: %s", function, hosts_allow_table);
#else
      yyerrorx_nolib("libwrap");
#endif /* HAVE_LIBWRAP */
#endif /* !SOCKS_CLIENT */
   }
#line 4044 "config_parse.c"
    break;

  case 148: /* libwrap_denyfile: LIBWRAP_DENY ':' LIBWRAP_FILE  */
#line 1376 "config_parse.y"
                                                {
#if !SOCKS_CLIENT
#if HAVE_LIBWRAP
      if ((hosts_deny_table  = strdup((yyvsp[0].string))) == NULL)
         yyerror(NOMEM);

      slog(LOG_DEBUG, "%s: libwrap.deny: %s", function, hosts_deny_table);
#else
      yyerrorx_nolib("libwrap");
#endif /* HAVE_LIBWRAP */
#endif /* !SOCKS_CLIENT */
   }
#line 4061 "config_parse.c"
    break;

  case 149: /* libwrap_hosts_access: LIBWRAP_HOSTS_ACCESS ':' YES  */
#line 1390 "config_parse.y"
                                                   {
#if !SOCKS_CLIENT
#if HAVE_LIBWRAP
      sockscf.option.hosts_access = 1;
#else
      yyerrorx("libwrap.hosts_access requires libwrap library");
#endif /* HAVE_LIBWRAP */
   }
#line 4074 "config_parse.c"
    break;

  case 150: /* libwrap_hosts_access: LIBWRAP_HOSTS_ACCESS ':' NO  */
#line 1398 "config_parse.y"
                                 {
#if HAVE_LIBWRAP
      sockscf.option.hosts_access = 0;
#else
      yyerrorx_nolib("libwrap");
#endif /* HAVE_LIBWRAP */
#endif /* !SOCKS_CLIENT */
   }
#line 4087 "config_parse.c"
    break;

  case 151: /* udpconnectdst: UDPCONNECTDST ':' YES  */
#line 1408 "config_parse.y"
                                     {
#if !SOCKS_CLIENT
      sockscf.udpconnectdst = 1;
   }
#line 4096 "config_parse.c"
    break;

  case 152: /* udpconnectdst: UDPCONNECTDST ':' NO  */
#line 1412 "config_parse.y"
                          {
      sockscf.udpconnectdst = 0;
#endif /* !SOCKS_CLIENT */
   }
#line 4105 "config_parse.c"
    break;

  case 154: /* compatibilityname: SAMEPORT  */
#line 1422 "config_parse.y"
                            {
#if !SOCKS_CLIENT
      sockscf.compat.sameport = 1;
   }
#line 4114 "config_parse.c"
    break;

  case 155: /* compatibilityname: DRAFT_5_05  */
#line 1426 "config_parse.y"
                 {
      sockscf.compat.draft_5_05 = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 4123 "config_parse.c"
    break;

  case 159: /* resolveprotocolname: PROTOCOL_FAKE  */
#line 1439 "config_parse.y"
                                     {
         sockscf.resolveprotocol = RESOLVEPROTOCOL_FAKE;
   }
#line 4131 "config_parse.c"
    break;

  case 160: /* resolveprotocolname: PROTOCOL_TCP  */
#line 1442 "config_parse.y"
                   {
#if HAVE_NO_RESOLVESTUFF
         yyerrorx("resolveprotocol keyword not supported on this system");
#else
         sockscf.resolveprotocol = RESOLVEPROTOCOL_TCP;
#endif /* !HAVE_NO_RESOLVESTUFF */
   }
#line 4143 "config_parse.c"
    break;

  case 161: /* resolveprotocolname: PROTOCOL_UDP  */
#line 1449 "config_parse.y"
                    {
         sockscf.resolveprotocol = RESOLVEPROTOCOL_UDP;
   }
#line 4151 "config_parse.c"
    break;

  case 164: /* cpuschedule: CPU '.' SCHEDULE '.' PROCESSTYPE ':' SCHEDULEPOLICY '/' NUMBER  */
#line 1458 "config_parse.y"
                                                                            {
#if !SOCKS_CLIENT
#if !HAVE_SCHED_SETSCHEDULER
      yyerrorx("cpu scheduling policy is not supported on this system");
#else /* HAVE_SCHED_SETSCHEDULER */
      cpusetting_t *cpusetting;

      switch ((yyvsp[-4].number)) {
         case PROC_MOTHER:
            cpusetting = &sockscf.cpu.mother;
            break;

         case PROC_MONITOR:
            cpusetting = &sockscf.cpu.monitor;
            break;

         case PROC_NEGOTIATE:
            cpusetting = &sockscf.cpu.negotiate;
            break;

         case PROC_REQUEST:
            cpusetting = &sockscf.cpu.request;
            break;

         case PROC_IO:
            cpusetting = &sockscf.cpu.io;
            break;

         default:
            SERRX((yyvsp[-4].number));
      }

      bzero(&cpusetting->param, sizeof(cpusetting->param));

      cpusetting->scheduling_isset     = 1;
      cpusetting->policy               = (yyvsp[-2].number);
      cpusetting->param.sched_priority = (int)(yyvsp[0].number);
#endif /* HAVE_SCHED_SETSCHEDULER */
#endif /* !SOCKS_CLIENT */
   }
#line 4196 "config_parse.c"
    break;

  case 165: /* cpuaffinity: CPU '.' MASK '.' PROCESSTYPE ':' numbers  */
#line 1500 "config_parse.y"
                                                      {
#if !SOCKS_CLIENT
#if !HAVE_SCHED_SETAFFINITY
      yyerrorx("cpu scheduling affinity is not supported on this system");
#else /* HAVE_SCHED_SETAFFINITY */
      cpusetting_t *cpusetting;

      switch ((yyvsp[-2].number)) {
         case PROC_MOTHER:
            cpusetting = &sockscf.cpu.mother;
            break;

         case PROC_MONITOR:
            cpusetting = &sockscf.cpu.monitor;
            break;

         case PROC_NEGOTIATE:
            cpusetting = &sockscf.cpu.negotiate;
            break;

         case PROC_REQUEST:
            cpusetting = &sockscf.cpu.request;
            break;

         case PROC_IO:
            cpusetting = &sockscf.cpu.io;
            break;

         default:
            SERRX((yyvsp[-2].number));
      }

      cpu_zero(&cpusetting->mask);
      while (numberc-- > 0)
         if (numberv[numberc] == CPUMASK_ANYCPU) {
            const long cpus = sysconf(_SC_NPROCESSORS_ONLN);
            long i;

            if (cpus == -1)
               yyerror("sysconf(_SC_NPROCESSORS_ONLN) failed");

            for (i = 0; i < cpus; ++i)
               cpu_set((int)i, &cpusetting->mask);
         }
         else if (numberv[numberc] < 0)
            yyerrorx("invalid CPU number: %ld.  The CPU number can not be "
                     "negative", (long)numberv[numberc]);
         else
            cpu_set(numberv[numberc], &cpusetting->mask);

      free(numberv);
      numberv = NULL;
      numberc = 0;

      cpusetting->affinity_isset = 1;

#endif /* HAVE_SCHED_SETAFFINITY */
#endif /* !SOCKS_CLIENT */
   }
#line 4260 "config_parse.c"
    break;

  case 166: /* $@14: %empty  */
#line 1561 "config_parse.y"
                                            {
#if !SOCKS_CLIENT
      socketopt.level = (yyvsp[-1].number);
#endif /* !SOCKS_CLIENT */
   }
#line 4270 "config_parse.c"
    break;

  case 168: /* socketoptionname: NUMBER  */
#line 1568 "config_parse.y"
                         {
#if !SOCKS_CLIENT
   socketopt.optname = (yyvsp[0].number);
   socketopt.info    = optval2sockopt(socketopt.level, socketopt.optname);

   if (socketopt.info == NULL)
      slog(LOG_DEBUG,
           "%s: unknown/unsupported socket option: level %d, value %d",
           function, socketopt.level, socketopt.optname);
   else
      socketoptioncheck(&socketopt);
   }
#line 4287 "config_parse.c"
    break;

  case 169: /* socketoptionname: SOCKETOPTION_OPTID  */
#line 1580 "config_parse.y"
                        {
      socketopt.info           = optid2sockopt((size_t)(yyvsp[0].number));
      SASSERTX(socketopt.info != NULL);

      socketopt.optname        = socketopt.info->value;

      socketoptioncheck(&socketopt);
#endif /* !SOCKS_CLIENT */
   }
#line 4301 "config_parse.c"
    break;

  case 170: /* socketoptionvalue: NUMBER  */
#line 1591 "config_parse.y"
                          {
      socketopt.optval.int_val = (int)(yyvsp[0].number);
      socketopt.opttype        = int_val;
   }
#line 4310 "config_parse.c"
    break;

  case 171: /* socketoptionvalue: SOCKETOPTION_SYMBOLICVALUE  */
#line 1595 "config_parse.y"
                                {
      const sockoptvalsym_t *p;

      if (socketopt.info == NULL)
         yyerrorx("the given socket option is unknown, so can not lookup "
                  "symbolic option value");

      if ((p = optval2valsym(socketopt.info->optid, (yyvsp[0].string))) == NULL)
         yyerrorx("symbolic value \"%s\" is unknown for socket option %s",
                  (yyvsp[0].string), sockopt2string(&socketopt, NULL, 0));

      socketopt.optval  = p->symval;
      socketopt.opttype = socketopt.info->opttype;
   }
#line 4329 "config_parse.c"
    break;

  case 172: /* socketside: INTERNALSOCKET  */
#line 1612 "config_parse.y"
                           { bzero(&socketopt, sizeof(socketopt));
                             socketopt.isinternalside = 1;
   }
#line 4337 "config_parse.c"
    break;

  case 173: /* socketside: EXTERNALSOCKET  */
#line 1615 "config_parse.y"
                           { bzero(&socketopt, sizeof(socketopt));
                             socketopt.isinternalside = 0;
   }
#line 4345 "config_parse.c"
    break;

  case 175: /* srchostoption: NODNSMISMATCH  */
#line 1624 "config_parse.y"
                               {
#if !SOCKS_CLIENT
         sockscf.srchost.nodnsmismatch = 1;
   }
#line 4354 "config_parse.c"
    break;

  case 176: /* srchostoption: NODNSUNKNOWN  */
#line 1628 "config_parse.y"
                   {
         sockscf.srchost.nodnsunknown = 1;
   }
#line 4362 "config_parse.c"
    break;

  case 177: /* srchostoption: CHECKREPLYAUTH  */
#line 1631 "config_parse.y"
                     {
         sockscf.srchost.checkreplyauth = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 4371 "config_parse.c"
    break;

  case 180: /* realm: REALM ':' REALNAME  */
#line 1641 "config_parse.y"
                          {
#if COVENANT
   STRCPY_CHECKLEN(sockscf.realmname,
                   (yyvsp[0].string),
                   sizeof(sockscf.realmname) - 1,
                   yyerrorx);
#else /* !COVENANT */
   yyerrorx("unknown keyword \"%s\"", (yyvsp[-2].string));
#endif /* !COVENANT */
}
#line 4386 "config_parse.c"
    break;

  case 181: /* $@15: %empty  */
#line 1653 "config_parse.y"
                                        {
#if !SOCKS_CLIENT

   cmethodv  = sockscf.cmethodv;
   cmethodc  = &sockscf.cmethodc;
  *cmethodc  = 0; /* reset. */

#endif /* !SOCKS_CLIENT */
   }
#line 4400 "config_parse.c"
    break;

  case 183: /* $@16: %empty  */
#line 1664 "config_parse.y"
                                      {
#if HAVE_SOCKS_RULES

      smethodv  = sockscf.smethodv;
      smethodc  = &sockscf.smethodc;
     *smethodc  = 0; /* reset. */

#else
      yyerrorx("\"socksmethod\" is not used in %s.  Only \"clientmethod\" "
               "is used",
               PRODUCT);
#endif /* !HAVE_SOCKS_RULES */
   }
#line 4418 "config_parse.c"
    break;

  case 188: /* socksmethodname: METHODNAME  */
#line 1686 "config_parse.y"
                            {
      if (methodisvalid((yyvsp[0].method), object_srule))
         ADDMETHOD((yyvsp[0].method), *smethodc, smethodv);
      else
         yyerrorx("method %s (%d) is not a valid method for socksmethods",
                  method2string((yyvsp[0].method)), (yyvsp[0].method));
   }
#line 4430 "config_parse.c"
    break;

  case 192: /* clientmethodname: METHODNAME  */
#line 1703 "config_parse.y"
                               {
      if (methodisvalid((yyvsp[0].method), object_crule))
         ADDMETHOD((yyvsp[0].method), *cmethodc, cmethodv);
      else
         yyerrorx("method %s (%d) is not a valid method for clientmethods",
                  method2string((yyvsp[0].method)), (yyvsp[0].method));
   }
#line 4442 "config_parse.c"
    break;

  case 193: /* $@17: %empty  */
#line 1711 "config_parse.y"
                 { objecttype = object_monitor; }
#line 4448 "config_parse.c"
    break;

  case 194: /* $@18: %empty  */
#line 1711 "config_parse.y"
                                                      {
#if !SOCKS_CLIENT
                        monitorinit(&monitor);
#endif /* !SOCKS_CLIENT */
}
#line 4458 "config_parse.c"
    break;

  case 195: /* monitor: MONITOR $@17 '{' $@18 monitoroptions fromto monitoroptions '}'  */
#line 1716 "config_parse.y"
{
#if !SOCKS_CLIENT
   pre_addmonitor(&monitor);

   addmonitor(&monitor);
#endif /* !SOCKS_CLIENT */
}
#line 4470 "config_parse.c"
    break;

  case 196: /* $@19: %empty  */
#line 1728 "config_parse.y"
                  { objecttype = object_crule; }
#line 4476 "config_parse.c"
    break;

  case 197: /* crule: CLIENTRULE $@19 verdict '{' cruleoptions fromto cruleoptions '}'  */
#line 1729 "config_parse.y"
                                                       {
#if !SOCKS_CLIENT
#if BAREFOOTD
      if (bounceto.atype == SOCKS_ADDR_NOTSET) {
         if (rule.verdict == VERDICT_PASS)
            yyerrorx("no address traffic should bounce to has been given");
         else {
            /*
             * allow no bounce-to address if it is a block, as the bounce-to
             * address will not be used in any case then.
             */
            bounceto.atype               = SOCKS_ADDR_IPV4;
            bounceto.addr.ipv4.ip.s_addr = htonl(INADDR_ANY);
            bounceto.port.tcp            = htons(0);
            bounceto.port.udp            = htons(0);
         }
      }

      rule.extra.bounceto = bounceto;
#endif /* BAREFOOTD */

      pre_addrule(&rule);
      addclientrule(&rule);
      post_addrule();
#endif /* !SOCKS_CLIENT */
   }
#line 4507 "config_parse.c"
    break;

  case 201: /* monitorside: %empty  */
#line 1762 "config_parse.y"
             {
#if !SOCKS_CLIENT
         monitorif = NULL;
   }
#line 4516 "config_parse.c"
    break;

  case 202: /* monitorside: ALARMIF_INTERNAL  */
#line 1766 "config_parse.y"
                       {
         monitorif = &monitor.mstats->object.monitor.internal;
   }
#line 4524 "config_parse.c"
    break;

  case 203: /* monitorside: ALARMIF_EXTERNAL  */
#line 1769 "config_parse.y"
                      {
         monitorif = &monitor.mstats->object.monitor.external;
#endif /* !SOCKS_CLIENT */
   }
#line 4533 "config_parse.c"
    break;

  case 204: /* alarmside: %empty  */
#line 1775 "config_parse.y"
           {
#if !SOCKS_CLIENT
      alarmside = NULL;
   }
#line 4542 "config_parse.c"
    break;

  case 205: /* alarmside: RECVSIDE  */
#line 1779 "config_parse.y"
              {
      *alarmside = RECVSIDE;
   }
#line 4550 "config_parse.c"
    break;

  case 206: /* alarmside: SENDSIDE  */
#line 1782 "config_parse.y"
              {
      *alarmside = SENDSIDE;
#endif /* !SOCKS_CLIENT */
   }
#line 4559 "config_parse.c"
    break;

  case 207: /* $@20: %empty  */
#line 1788 "config_parse.y"
                                       { alarminit(); }
#line 4565 "config_parse.c"
    break;

  case 208: /* alarm_data: monitorside ALARMTYPE_DATA $@20 alarmside ':' NUMBER WORD__IN NUMBER  */
#line 1789 "config_parse.y"
                                    {
#if !SOCKS_CLIENT
   alarm_data_limit_t limit;

   ASSIGN_NUMBER((yyvsp[-2].number), >=, 0, limit.bytes, 0);
   ASSIGN_NUMBER((yyvsp[0].number), >, 0, limit.seconds, 1);

   monitor.alarmsconfigured |= ALARM_DATA;

   if (monitor.alarm_data_aggregate != 0)
      yyerrorx("one aggregated data alarm has already been specified.  "
               "No more data alarms can be specified in this monitor");

   if (monitorif == NULL) {
      monitor.alarm_data_aggregate = ALARM_INTERNAL | ALARM_EXTERNAL;

      if (alarmside == NULL)
         monitor.alarm_data_aggregate |= ALARM_RECV | ALARM_SEND;

      if (alarmside == NULL || *alarmside == RECVSIDE) {
         monitor.mstats->object.monitor.internal.alarm.data.recv.isconfigured
         = 1;
         monitor.mstats->object.monitor.internal.alarm.data.recv.limit = limit;
      }

      if (alarmside == NULL || *alarmside == SENDSIDE) {
         monitor.mstats->object.monitor.internal.alarm.data.send.isconfigured
         = 1;
         monitor.mstats->object.monitor.internal.alarm.data.send.limit = limit;
      }

      if (alarmside == NULL || *alarmside == RECVSIDE) {
         monitor.mstats->object.monitor.external.alarm.data.recv.isconfigured
         = 1;
         monitor.mstats->object.monitor.external.alarm.data.recv.limit = limit;
      }

      if (alarmside == NULL || *alarmside == SENDSIDE) {
         monitor.mstats->object.monitor.external.alarm.data.send.isconfigured
         = 1;
         monitor.mstats->object.monitor.external.alarm.data.send.limit = limit;
      }
   }
   else {
      if (alarmside == NULL)
         monitor.alarm_data_aggregate = ALARM_RECV | ALARM_SEND;

      if (alarmside == NULL || *alarmside == RECVSIDE) {
         monitorif->alarm.data.recv.isconfigured = 1;
         monitorif->alarm.data.recv.limit        = limit;
      }

      if (alarmside == NULL || *alarmside == SENDSIDE) {
         monitorif->alarm.data.send.isconfigured = 1;
         monitorif->alarm.data.send.limit        = limit;
      }
   }
#endif /* !SOCKS_CLIENT */
   }
#line 4629 "config_parse.c"
    break;

  case 210: /* networkproblem: MTU_ERROR  */
#line 1853 "config_parse.y"
                          {
#if !SOCKS_CLIENT
   monitor.alarmsconfigured |= ALARM_TEST;

   if (monitorif == NULL) {
      monitor.mstats->object.monitor.internal.alarm.test.mtu.dotest = 1;
      monitor.mstats->object.monitor.external.alarm.test.mtu.dotest = 1;
   }
   else {
      monitorif->alarm.test.mtu.dotest = 1;
      monitorif->alarm.test.mtu.dotest = 1;
   }
#endif /* !SOCKS_CLIENT */
   }
#line 4648 "config_parse.c"
    break;

  case 211: /* alarm_disconnect: monitorside ALARMTYPE_DISCONNECT ':' NUMBER '/' NUMBER alarmperiod  */
#line 1871 "config_parse.y"
                                                                      {
#if !SOCKS_CLIENT
   alarm_disconnect_limit_t limit;

   ASSIGN_NUMBER((yyvsp[-1].number), >, 0, limit.sessionc, 0);
   ASSIGN_NUMBER((yyvsp[-3].number), >, 0, limit.disconnectc, 0);
   ASSIGN_NUMBER((yyvsp[0].number), >, 0, limit.seconds, 1);

   if (monitor.alarm_disconnect_aggregate != 0)
      yyerrorx("one aggregated disconnect alarm has already been specified.  "
               "No more disconnect alarms can be specified in this monitor");

   monitor.alarmsconfigured |= ALARM_DISCONNECT;

   if (monitorif == NULL) {
      monitor.alarm_disconnect_aggregate = ALARM_INTERNAL | ALARM_EXTERNAL;

      monitor.mstats->object.monitor.internal.alarm.disconnect.isconfigured = 1;
      monitor.mstats->object.monitor.internal.alarm.disconnect.limit = limit;

        monitor.mstats->object.monitor.external.alarm.disconnect
      = monitor.mstats->object.monitor.internal.alarm.disconnect;
   }
   else {
      monitorif->alarm.disconnect.isconfigured = 1;
      monitorif->alarm.disconnect.limit        = limit;
   }
#endif /* !SOCKS_CLIENT */
   }
#line 4682 "config_parse.c"
    break;

  case 212: /* alarmperiod: %empty  */
#line 1902 "config_parse.y"
             {
#if !SOCKS_CLIENT
               (yyval.number) = DEFAULT_ALARM_PERIOD;
#endif /* !SOCKS_CLIENT */
   }
#line 4692 "config_parse.c"
    break;

  case 213: /* alarmperiod: WORD__IN NUMBER  */
#line 1907 "config_parse.y"
                     { (yyval.number) = (yyvsp[0].number); }
#line 4698 "config_parse.c"
    break;

  case 216: /* monitoroption: hostidoption  */
#line 1912 "config_parse.y"
                            { *hostidoption_isset = 1; }
#line 4704 "config_parse.c"
    break;

  case 218: /* monitoroptions: %empty  */
#line 1916 "config_parse.y"
                  { (yyval.string) = NULL; }
#line 4710 "config_parse.c"
    break;

  case 220: /* cruleoption: bounce  */
#line 1920 "config_parse.y"
                     {
#if !BAREFOOTD
                  yyerrorx("unsupported option");
#endif /* !BAREFOOTD */
   }
#line 4720 "config_parse.c"
    break;

  case 221: /* cruleoption: protocol  */
#line 1925 "config_parse.y"
                      {
#if !BAREFOOTD
                  yyerrorx("unsupported option");
#endif /* !BAREFOOTD */
   }
#line 4730 "config_parse.c"
    break;

  case 223: /* cruleoption: crulesessionoption  */
#line 1931 "config_parse.y"
                                {
#if !SOCKS_CLIENT
                  session_isset = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 4740 "config_parse.c"
    break;

  case 225: /* $@21: %empty  */
#line 1939 "config_parse.y"
                  {

#if SOCKS_CLIENT || !HAVE_SOCKS_HOSTID
      yyerrorx("hostid is not supported on this system");
#endif /* SOCKS_CLIENT || !HAVE_SOCKS_HOSTID */

      objecttype = object_hrule;
}
#line 4753 "config_parse.c"
    break;

  case 226: /* hrule: HOSTIDRULE $@21 verdict '{' cruleoptions hostid_fromto cruleoptions '}'  */
#line 1946 "config_parse.y"
                                                          {
#if !SOCKS_CLIENT && HAVE_SOCKS_HOSTID
      if (hostid.atype != SOCKS_ADDR_NOTSET)
         yyerrorx("it does not make sense to set the hostid address in a "
                  "hostid-rule.  Use the \"from\" address to match the hostid "
                  "of the client");

      *hostidoption_isset = 1;

      pre_addrule(&rule);
      addhostidrule(&rule);
      post_addrule();
#endif /* !SOCKS_CLIENT && HAVE_SOCKS_HOSTID */
   }
#line 4772 "config_parse.c"
    break;

  case 227: /* cruleoptions: %empty  */
#line 1964 "config_parse.y"
                { (yyval.string) = NULL; }
#line 4778 "config_parse.c"
    break;

  case 231: /* $@22: %empty  */
#line 1972 "config_parse.y"
                   {
#if !SOCKS_CLIENT && HAVE_SOCKS_HOSTID
      addrinit(&hostid, 1);

#else /* HAVE_SOCKS_HOSTID */
      yyerrorx("hostid is not supported on this system");
#endif /* HAVE_SOCKS_HOSTID */

   }
#line 4792 "config_parse.c"
    break;

  case 233: /* hostindex: HOSTINDEX ':' NUMBER  */
#line 1983 "config_parse.y"
                                {
#if !SOCKS_CLIENT && HAVE_SOCKS_HOSTID
   ASSIGN_NUMBER((yyvsp[0].number), >=, 0, *hostindex, 0);
   ASSIGN_NUMBER((yyvsp[0].number), <=, HAVE_MAX_HOSTIDS, *hostindex, 0);

#else
   yyerrorx("hostid is not supported on this system");
#endif /* !SOCKS_CLIENT && HAVE_SOCKS_HOSTID */
}
#line 4806 "config_parse.c"
    break;

  case 234: /* $@23: %empty  */
#line 1995 "config_parse.y"
                 { objecttype = object_srule; }
#line 4812 "config_parse.c"
    break;

  case 235: /* srule: SOCKSRULE $@23 verdict '{' sruleoptions fromto sruleoptions '}'  */
#line 1996 "config_parse.y"
                                                      {
#if !SOCKS_CLIENT
#if !HAVE_SOCKS_RULES
   yyerrorx("socks-rules are not used in %s", PRODUCT);
#endif /* !HAVE_SOCKS_RULES */

      pre_addrule(&rule);
      addsocksrule(&rule);
      post_addrule();
#endif /* !SOCKS_CLIENT */
   }
#line 4828 "config_parse.c"
    break;

  case 236: /* sruleoptions: %empty  */
#line 2010 "config_parse.y"
                { (yyval.string) = NULL; }
#line 4834 "config_parse.c"
    break;

  case 245: /* sruleoption: sockssessionoption  */
#line 2022 "config_parse.y"
                                {
#if !SOCKS_CLIENT
                  session_isset = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 4844 "config_parse.c"
    break;

  case 247: /* genericruleoption: bandwidth  */
#line 2031 "config_parse.y"
                              {
#if !SOCKS_CLIENT
                        checkmodule("bandwidth");
                        bw_isset = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 4855 "config_parse.c"
    break;

  case 255: /* genericruleoption: hostidoption  */
#line 2044 "config_parse.y"
                         { *hostidoption_isset = 1; }
#line 4861 "config_parse.c"
    break;

  case 260: /* genericruleoption: psid  */
#line 2049 "config_parse.y"
            {
#if !SOCKS_CLIENT
                     checkmodule("pac");
#endif /* !SOCKS_CLIENT */
   }
#line 4871 "config_parse.c"
    break;

  case 261: /* genericruleoption: psid_b64  */
#line 2054 "config_parse.y"
                {
#if !SOCKS_CLIENT
                     checkmodule("pac");
#endif /* !SOCKS_CLIENT */
   }
#line 4881 "config_parse.c"
    break;

  case 262: /* genericruleoption: psid_off  */
#line 2059 "config_parse.y"
                     {

#if !SOCKS_CLIENT

                     checkmodule("pac");

#endif /* !SOCKS_CLIENT */
   }
#line 4894 "config_parse.c"
    break;

  case 263: /* genericruleoption: redirect  */
#line 2067 "config_parse.y"
                       {
#if !SOCKS_CLIENT
                     checkmodule("redirect");
#endif /* !SOCKS_CLIENT */
   }
#line 4904 "config_parse.c"
    break;

  case 264: /* genericruleoption: socketoption  */
#line 2072 "config_parse.y"
                         {
#if !SOCKS_CLIENT
         if (rule.verdict == VERDICT_BLOCK && !socketopt.isinternalside)
            yyerrorx("it does not make sense to set a socket option for the "
                     "external side in a rule that blocks access; the external "
                     "side will never be accessed as the rule blocks access "
                     "to it");

         if (socketopt.isinternalside)
            if (socketopt.info != NULL && socketopt.info->calltype == preonly)
               yywarnx("to our knowledge the socket option \"%s\" can only be "
                       "correctly applied at pre-connection establishment "
                       "time, but by the time this rule is matched, the "
                       "connection will already have been established",
                       socketopt.info == NULL ? "unknown" :
                                                socketopt.info->name);

         if (!addedsocketoption(&rule.socketoptionc,
                                &rule.socketoptionv,
                                &socketopt))
            yywarn("could not add socketoption");
#endif /* !SOCKS_CLIENT */
   }
#line 4932 "config_parse.c"
    break;

  case 312: /* ldapdebug: LDAPDEBUG ':' NUMBER  */
#line 2149 "config_parse.y"
                                {
#if SOCKS_SERVER
#if HAVE_LDAP && HAVE_OPENLDAP
      ldapauthorisation->debug = (int)(yyvsp[0].number);
   }
#line 4942 "config_parse.c"
    break;

  case 313: /* ldapdebug: LDAPDEBUG ':' '-' NUMBER  */
#line 2154 "config_parse.y"
                             {
      ldapauthorisation->debug = (int)-(yyvsp[0].number);
 #else /* !HAVE_LDAP */
      yyerrorx_nolib("openldap");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 4954 "config_parse.c"
    break;

  case 314: /* ldapauthdebug: LDAPAUTHDEBUG ':' NUMBER  */
#line 2163 "config_parse.y"
                                        {
#if SOCKS_SERVER
#if HAVE_LDAP && HAVE_OPENLDAP
      ldapauthentication->debug = (int)(yyvsp[0].number);
   }
#line 4964 "config_parse.c"
    break;

  case 315: /* ldapauthdebug: LDAPAUTHDEBUG ':' '-' NUMBER  */
#line 2168 "config_parse.y"
                                 {
      ldapauthentication->debug = (int)-(yyvsp[0].number);
 #else /* !HAVE_LDAP */
      yyerrorx_nolib("openldap");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 4976 "config_parse.c"
    break;

  case 316: /* ldapdomain: LDAPDOMAIN ':' LDAP_DOMAIN  */
#line 2177 "config_parse.y"
                                       {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKLEN(state->ldapauthorisation.domain,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthorisation.domain) - 1,
                      yyerrorx);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 4993 "config_parse.c"
    break;

  case 317: /* ldapauthdomain: LDAPAUTHDOMAIN ':' LDAP_DOMAIN  */
#line 2191 "config_parse.y"
                                               {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKLEN(state->ldapauthentication.domain,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthentication.domain) - 1,
                      yyerrorx);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5010 "config_parse.c"
    break;

  case 318: /* ldapdepth: LDAPDEPTH ':' NUMBER  */
#line 2205 "config_parse.y"
                                {
#if SOCKS_SERVER
#if HAVE_LDAP && HAVE_OPENLDAP
      ldapauthorisation->mdepth = (int)(yyvsp[0].number);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("openldap");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5024 "config_parse.c"
    break;

  case 319: /* ldapcertfile: LDAPCERTFILE ':' LDAP_CERTFILE  */
#line 2216 "config_parse.y"
                                             {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKLEN(state->ldapauthorisation.certfile,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthorisation.certfile) - 1,
                      yyerrorx);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5041 "config_parse.c"
    break;

  case 320: /* ldapauthcertfile: LDAPAUTHCERTFILE ':' LDAP_CERTFILE  */
#line 2230 "config_parse.y"
                                                     {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKLEN(state->ldapauthentication.certfile,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthentication.certfile) - 1,
                      yyerrorx);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5058 "config_parse.c"
    break;

  case 321: /* ldapcertpath: LDAPCERTPATH ':' LDAP_CERTPATH  */
#line 2244 "config_parse.y"
                                             {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKLEN(state->ldapauthorisation.certpath,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthorisation.certpath) - 1,
                      yyerrorx);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5075 "config_parse.c"
    break;

  case 322: /* ldapauthcertpath: LDAPAUTHCERTPATH ':' LDAP_CERTPATH  */
#line 2258 "config_parse.y"
                                                     {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKLEN(state->ldapauthentication.certpath,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthentication.certpath) - 1,
                      yyerrorx);
#else /* !HAVE_LDAP */

      yyerrorx_nolib("LDAP");

#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5094 "config_parse.c"
    break;

  case 323: /* ldapurl: LDAPURL ':' LDAP_URL  */
#line 2274 "config_parse.y"
                              {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthorisation.ldapurl, (yyvsp[0].string)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5109 "config_parse.c"
    break;

  case 324: /* ldapauthurl: LDAPAUTHURL ':' LDAP_URL  */
#line 2286 "config_parse.y"
                                      {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthentication.ldapurl, (yyvsp[0].string)) == NULL)
         yyerror(NOMEM);
      if (sockscf.state.ldapauthentication.ldapurl == NULL)
         sockscf.state.ldapauthentication.ldapurl = state->ldapauthentication.ldapurl;
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5126 "config_parse.c"
    break;

  case 325: /* ldapauthbasedn: LDAPAUTHBASEDN ':' LDAP_BASEDN  */
#line 2300 "config_parse.y"
                                               {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthentication.ldapbasedn, (yyvsp[0].string)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5141 "config_parse.c"
    break;

  case 326: /* ldapauthbasedn_hex: LDAPAUTHBASEDN_HEX ':' LDAP_BASEDN  */
#line 2312 "config_parse.y"
                                                       {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthentication.ldapbasedn, hextoutf8((yyvsp[0].string), 0)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5156 "config_parse.c"
    break;

  case 327: /* ldapauthbasedn_hex_all: LDAPAUTHBASEDN_HEX_ALL ':' LDAP_BASEDN  */
#line 2324 "config_parse.y"
                                                               {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthentication.ldapbasedn, hextoutf8((yyvsp[0].string), 1)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5171 "config_parse.c"
    break;

  case 328: /* lbasedn: LDAPBASEDN ':' LDAP_BASEDN  */
#line 2336 "config_parse.y"
                                    {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthorisation.ldapbasedn, (yyvsp[0].string)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5186 "config_parse.c"
    break;

  case 329: /* lbasedn_hex: LDAPBASEDN_HEX ':' LDAP_BASEDN  */
#line 2348 "config_parse.y"
                                            {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthorisation.ldapbasedn, hextoutf8((yyvsp[0].string), 0)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5201 "config_parse.c"
    break;

  case 330: /* lbasedn_hex_all: LDAPBASEDN_HEX_ALL ':' LDAP_BASEDN  */
#line 2360 "config_parse.y"
                                                    {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthorisation.ldapbasedn, hextoutf8((yyvsp[0].string), 1)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5216 "config_parse.c"
    break;

  case 331: /* ldapauthport: LDAPAUTHPORT ':' NUMBER  */
#line 2372 "config_parse.y"
                                      {
#if SOCKS_SERVER
#if HAVE_LDAP
   ldapauthentication->port = (int)(yyvsp[0].number);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5230 "config_parse.c"
    break;

  case 332: /* ldapport: LDAPPORT ':' NUMBER  */
#line 2383 "config_parse.y"
                              {
#if SOCKS_SERVER
#if HAVE_LDAP
   ldapauthorisation->port = (int)(yyvsp[0].number);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5244 "config_parse.c"
    break;

  case 333: /* ldapauthportssl: LDAPAUTHPORTSSL ':' NUMBER  */
#line 2394 "config_parse.y"
                                            {
#if SOCKS_SERVER
#if HAVE_LDAP
   ldapauthentication->portssl = (int)(yyvsp[0].number);
#else /* !HAVE_LDAP */
   yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5258 "config_parse.c"
    break;

  case 334: /* ldapportssl: LDAPPORTSSL ':' NUMBER  */
#line 2405 "config_parse.y"
                                    {
#if SOCKS_SERVER
#if HAVE_LDAP
   ldapauthorisation->portssl = (int)(yyvsp[0].number);
#else /* !HAVE_LDAP */
   yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5272 "config_parse.c"
    break;

  case 335: /* ldapssl: LDAPSSL ':' YES  */
#line 2416 "config_parse.y"
                         {
#if SOCKS_SERVER
#if HAVE_LDAP
      ldapauthorisation->ssl = 1;
   }
#line 5282 "config_parse.c"
    break;

  case 336: /* ldapssl: LDAPSSL ':' NO  */
#line 2421 "config_parse.y"
                    {
      ldapauthorisation->ssl = 0;
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5294 "config_parse.c"
    break;

  case 337: /* ldapauthssl: LDAPAUTHSSL ':' YES  */
#line 2430 "config_parse.y"
                                 {
#if SOCKS_SERVER
#if HAVE_LDAP
      ldapauthentication->ssl = 1;
   }
#line 5304 "config_parse.c"
    break;

  case 338: /* ldapauthssl: LDAPAUTHSSL ':' NO  */
#line 2435 "config_parse.y"
                        {
      ldapauthentication->ssl = 0;
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5316 "config_parse.c"
    break;

  case 339: /* ldapauto: LDAPAUTO ':' YES  */
#line 2444 "config_parse.y"
                           {
#if SOCKS_SERVER
#if HAVE_LDAP
      ldapauthorisation->auto_off = 1;
   }
#line 5326 "config_parse.c"
    break;

  case 340: /* ldapauto: LDAPAUTO ':' NO  */
#line 2449 "config_parse.y"
                     {
      ldapauthorisation->auto_off = 0;
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5338 "config_parse.c"
    break;

  case 341: /* ldapauthauto: LDAPAUTHAUTO ':' YES  */
#line 2458 "config_parse.y"
                                   {
#if SOCKS_SERVER
#if HAVE_LDAP
      ldapauthentication->auto_off = 1;
   }
#line 5348 "config_parse.c"
    break;

  case 342: /* ldapauthauto: LDAPAUTHAUTO ':' NO  */
#line 2463 "config_parse.y"
                         {
      ldapauthentication->auto_off = 0;
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5360 "config_parse.c"
    break;

  case 343: /* ldapcertcheck: LDAPCERTCHECK ':' YES  */
#line 2472 "config_parse.y"
                                      {
#if SOCKS_SERVER
#if HAVE_LDAP
      ldapauthorisation->certcheck = 1;
   }
#line 5370 "config_parse.c"
    break;

  case 344: /* ldapcertcheck: LDAPCERTCHECK ':' NO  */
#line 2477 "config_parse.y"
                          {
      ldapauthorisation->certcheck = 0;
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5382 "config_parse.c"
    break;

  case 345: /* ldapauthcertcheck: LDAPAUTHCERTCHECK ':' YES  */
#line 2486 "config_parse.y"
                                              {
#if SOCKS_SERVER
#if HAVE_LDAP
      ldapauthentication->certcheck = 1;
   }
#line 5392 "config_parse.c"
    break;

  case 346: /* ldapauthcertcheck: LDAPAUTHCERTCHECK ':' NO  */
#line 2491 "config_parse.y"
                              {
      ldapauthentication->certcheck = 0;
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5404 "config_parse.c"
    break;

  case 347: /* ldapauthkeeprealm: LDAPAUTHKEEPREALM ':' YES  */
#line 2500 "config_parse.y"
                                              {
#if SOCKS_SERVER
#if HAVE_LDAP
      ldapauthentication->keeprealm = 1;
   }
#line 5414 "config_parse.c"
    break;

  case 348: /* ldapauthkeeprealm: LDAPAUTHKEEPREALM ':' NO  */
#line 2505 "config_parse.y"
                              {
      ldapauthentication->keeprealm = 0;
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5426 "config_parse.c"
    break;

  case 349: /* ldapkeeprealm: LDAPKEEPREALM ':' YES  */
#line 2515 "config_parse.y"
                                      {
#if SOCKS_SERVER
#if HAVE_LDAP
      ldapauthorisation->keeprealm = 1;
   }
#line 5436 "config_parse.c"
    break;

  case 350: /* ldapkeeprealm: LDAPKEEPREALM ':' NO  */
#line 2520 "config_parse.y"
                          {
      ldapauthorisation->keeprealm = 0;
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5448 "config_parse.c"
    break;

  case 351: /* ldapfilter: LDAPFILTER ':' LDAP_FILTER  */
#line 2529 "config_parse.y"
                                       {
#if SOCKS_SERVER
#if HAVE_LDAP
   STRCPY_CHECKLEN(ldapauthorisation->filter, (yyvsp[0].string), sizeof(state->ldapauthorisation.filter) - 1, yyerrorx);
#else /* !HAVE_LDAP */
   yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5462 "config_parse.c"
    break;

  case 352: /* ldapauthfilter: LDAPAUTHFILTER ':' LDAP_FILTER  */
#line 2540 "config_parse.y"
                                               {
#if SOCKS_SERVER
#if HAVE_LDAP
   STRCPY_CHECKLEN(ldapauthentication->filter, (yyvsp[0].string), sizeof(state->ldapauthentication.filter) - 1, yyerrorx);
#else /* !HAVE_LDAP */
   yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5476 "config_parse.c"
    break;

  case 353: /* ldapfilter_ad: LDAPFILTER_AD ':' LDAP_FILTER  */
#line 2551 "config_parse.y"
                                             {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKLEN(ldapauthorisation->filter_AD,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthorisation.filter_AD) - 1,
                      yyerrorx);

#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5494 "config_parse.c"
    break;

  case 354: /* ldapfilter_hex: LDAPFILTER_HEX ':' LDAP_FILTER  */
#line 2566 "config_parse.y"
                                               {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKUTFLEN(ldapauthorisation->filter,
                          (yyvsp[0].string),
                          sizeof(state->ldapauthorisation.filter) - 1,
                          yyerrorx);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5511 "config_parse.c"
    break;

  case 355: /* ldapfilter_ad_hex: LDAPFILTER_AD_HEX ':' LDAP_FILTER  */
#line 2580 "config_parse.y"
                                                     {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKUTFLEN(ldapauthorisation->filter_AD,
                        (yyvsp[0].string),
                        sizeof(state->ldapauthorisation.filter_AD) - 1,
                        yyerrorx);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5528 "config_parse.c"
    break;

  case 356: /* ldapattribute: LDAPATTRIBUTE ':' LDAP_ATTRIBUTE  */
#line 2594 "config_parse.y"
                                                {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKLEN(ldapauthorisation->attribute,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthorisation.attribute) - 1,
                      yyerrorx);

#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5546 "config_parse.c"
    break;

  case 357: /* ldapattribute_ad: LDAPATTRIBUTE_AD ':' LDAP_ATTRIBUTE  */
#line 2609 "config_parse.y"
                                                      {
#if SOCKS_SERVER
#if HAVE_LDAP
      STRCPY_CHECKLEN(ldapauthorisation->attribute_AD,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthorisation.attribute_AD) - 1,
                      yyerrorx);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5563 "config_parse.c"
    break;

  case 358: /* ldapattribute_hex: LDAPATTRIBUTE_HEX ':' LDAP_ATTRIBUTE  */
#line 2623 "config_parse.y"
                                                        {
#if SOCKS_SERVER
#if HAVE_LDAP
   STRCPY_CHECKUTFLEN(ldapauthorisation->attribute,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthorisation.attribute) -1,
                      yyerrorx);
#else /* !HAVE_LDAP */
   yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5580 "config_parse.c"
    break;

  case 359: /* ldapattribute_ad_hex: LDAPATTRIBUTE_AD_HEX ':' LDAP_ATTRIBUTE  */
#line 2637 "config_parse.y"
                                                              {
#if SOCKS_SERVER
#if HAVE_LDAP
   STRCPY_CHECKUTFLEN(ldapauthorisation->attribute_AD,
                      (yyvsp[0].string),
                      sizeof(state->ldapauthorisation.attribute_AD) - 1,
                      yyerrorx);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5597 "config_parse.c"
    break;

  case 360: /* lgroup_hex: LDAPGROUP_HEX ':' LDAPGROUP_NAME  */
#line 2651 "config_parse.y"
                                             {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&rule.ldapgroup, hextoutf8((yyvsp[0].string), 0)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5612 "config_parse.c"
    break;

  case 361: /* lgroup_hex_all: LDAPGROUP_HEX_ALL ':' LDAPGROUP_NAME  */
#line 2663 "config_parse.y"
                                                     {
#if SOCKS_SERVER
#if HAVE_LDAP
      checkmodule("ldap");

      if (addlinkedname(&rule.ldapgroup, hextoutf8((yyvsp[0].string), 1)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5629 "config_parse.c"
    break;

  case 362: /* lgroup: LDAPGROUP ':' LDAPGROUP_NAME  */
#line 2677 "config_parse.y"
                                     {
#if SOCKS_SERVER
#if HAVE_LDAP
      checkmodule("ldap");

      if (addlinkedname(&rule.ldapgroup, asciitoutf8((yyvsp[0].string))) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5646 "config_parse.c"
    break;

  case 363: /* lserver: LDAPSERVER ':' LDAPSERVER_NAME  */
#line 2691 "config_parse.y"
                                        {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthorisation.ldapserver, (yyvsp[0].string)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5661 "config_parse.c"
    break;

  case 364: /* ldapauthserver: LDAPAUTHSERVER ':' LDAPSERVER_NAME  */
#line 2703 "config_parse.y"
                                                   {
#if SOCKS_SERVER
#if HAVE_LDAP
      if (addlinkedname(&state->ldapauthentication.ldapserver, (yyvsp[0].string)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("LDAP");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5676 "config_parse.c"
    break;

  case 365: /* ldapkeytab: LDAPKEYTAB ':' LDAPKEYTABNAME  */
#line 2715 "config_parse.y"
                                          {
#if HAVE_LDAP
#if SOCKS_SERVER
   STRCPY_CHECKLEN(state->ldapauthorisation.keytab,
                   (yyvsp[0].string),
                   sizeof(state->ldapauthorisation.keytab) - 1, yyerrorx);
#else
   yyerrorx("LDAP keytab only applicable to Dante server");
#endif /* SOCKS_SERVER */
#else
      yyerrorx_nolib("LDAP");
#endif /* HAVE_LDAP */
   }
#line 5694 "config_parse.c"
    break;

  case 366: /* ldapauthkeytab: LDAPAUTHKEYTAB ':' LDAPKEYTABNAME  */
#line 2730 "config_parse.y"
                                                  {
#if HAVE_LDAP
#if SOCKS_SERVER
   STRCPY_CHECKLEN(state->ldapauthentication.keytab,
                   (yyvsp[0].string),
                   sizeof(state->ldapauthentication.keytab) - 1, yyerrorx);
#else
   yyerrorx("LDAP keytab only applicable to Dante server");
#endif /* SOCKS_SERVER */
#else
      yyerrorx_nolib("LDAP");
#endif /* HAVE_LDAP */
   }
#line 5712 "config_parse.c"
    break;

  case 367: /* psid: PACSID ':' PACSID_NAME  */
#line 2745 "config_parse.y"
                             {
#if SOCKS_SERVER
#if HAVE_PAC
      char b64[MAX_BASE64_LEN];

      checkmodule("pac");

      if (sidtob64((yyvsp[0].string), b64, sizeof(b64)) != 0)
         yyerrorx("invalid input: %s)", (yyvsp[0].string));
      if (addlinkedname(&rule.objectsids, b64) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("PAC");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5733 "config_parse.c"
    break;

  case 368: /* psid_b64: PACSID_B64 ':' PACSID_NAME  */
#line 2763 "config_parse.y"
                                     {
#if SOCKS_SERVER
#if HAVE_PAC
      char sid[MAX_BASE64_LEN];
      checkmodule("pac");

      /* attempt conversion to check if input makes sense */
      if (b64tosid((yyvsp[0].string), sid, sizeof(sid)) != 0)
         yyerrorx("invalid input: %s)", (yyvsp[0].string));
      if (addlinkedname(&rule.objectsids, (yyvsp[0].string)) == NULL)
         yyerror(NOMEM);
#else /* !HAVE_LDAP */
      yyerrorx_nolib("PAC");
#endif /* !HAVE_LDAP */
#endif /* SOCKS_SERVER */
   }
#line 5754 "config_parse.c"
    break;

  case 369: /* psid_off: PACSID_FLAG ':' YES  */
#line 2781 "config_parse.y"
                              {
#if SOCKS_SERVER
#if HAVE_PAC
      checkmodule("pac");
      rule.pacoff = 1;
   }
#line 5765 "config_parse.c"
    break;

  case 370: /* psid_off: PACSID_FLAG ':' NO  */
#line 2787 "config_parse.y"
                        {
      checkmodule("pac");
      rule.pacoff = 0;
#else /* !HAVE_PAC */
      yyerrorx_nolib("PAC");
#endif /* !HAVE_PAC */
#endif /* SOCKS_SERVER */
   }
#line 5778 "config_parse.c"
    break;

  case 372: /* clientcompatibilityname: NECGSSAPI  */
#line 2800 "config_parse.y"
                                   {
#if HAVE_GSSAPI
      gssapiencryption->nec = 1;
#else
      yyerrorx_nolib("GSSAPI");
#endif /* HAVE_GSSAPI */
   }
#line 5790 "config_parse.c"
    break;

  case 375: /* verdict: VERDICT_BLOCK  */
#line 2814 "config_parse.y"
                         {
#if !SOCKS_CLIENT
      ruleinit(&rule);
      rule.verdict   = VERDICT_BLOCK;
   }
#line 5800 "config_parse.c"
    break;

  case 376: /* verdict: VERDICT_PASS  */
#line 2819 "config_parse.y"
                    {
      ruleinit(&rule);
      rule.verdict   = VERDICT_PASS;
#endif /* !SOCKS_CLIENT */
   }
#line 5810 "config_parse.c"
    break;

  case 380: /* commandname: COMMAND_BIND  */
#line 2833 "config_parse.y"
                            {
         state->command.bind = 1;
   }
#line 5818 "config_parse.c"
    break;

  case 381: /* commandname: COMMAND_CONNECT  */
#line 2836 "config_parse.y"
                       {
         state->command.connect = 1;
   }
#line 5826 "config_parse.c"
    break;

  case 382: /* commandname: COMMAND_UDPASSOCIATE  */
#line 2839 "config_parse.y"
                            {
         state->command.udpassociate = 1;
   }
#line 5834 "config_parse.c"
    break;

  case 383: /* commandname: COMMAND_BINDREPLY  */
#line 2845 "config_parse.y"
                           {
         state->command.bindreply = 1;
   }
#line 5842 "config_parse.c"
    break;

  case 384: /* commandname: COMMAND_UDPREPLY  */
#line 2849 "config_parse.y"
                        {
         state->command.udpreply = 1;
   }
#line 5850 "config_parse.c"
    break;

  case 388: /* protocolname: PROTOCOL_TCP  */
#line 2862 "config_parse.y"
                           {
      state->protocol.tcp = 1;
   }
#line 5858 "config_parse.c"
    break;

  case 389: /* protocolname: PROTOCOL_UDP  */
#line 2865 "config_parse.y"
                           {
      state->protocol.udp = 1;
   }
#line 5866 "config_parse.c"
    break;

  case 401: /* sessioninheritable: SESSION_INHERITABLE ':' YES  */
#line 2894 "config_parse.y"
                                                {
#if !SOCKS_CLIENT
                        rule.ss_isinheritable = 1;
   }
#line 5875 "config_parse.c"
    break;

  case 402: /* sessioninheritable: SESSION_INHERITABLE ':' NO  */
#line 2898 "config_parse.y"
                                {
                        rule.ss_isinheritable = 0;
#endif /* !SOCKS_CLIENT */
   }
#line 5884 "config_parse.c"
    break;

  case 403: /* sessionmax: SESSIONMAX ':' NUMBER  */
#line 2904 "config_parse.y"
                                  {
#if !SOCKS_CLIENT
      ASSIGN_MAXSESSIONS((yyvsp[0].number), ss.object.ss.max, 0);
      ss.object.ss.max       = (yyvsp[0].number);
      ss.object.ss.max_isset = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 5896 "config_parse.c"
    break;

  case 404: /* sessionthrottle: SESSIONTHROTTLE ':' NUMBER '/' NUMBER  */
#line 2913 "config_parse.y"
                                                       {
#if !SOCKS_CLIENT
      ASSIGN_THROTTLE_SECONDS((yyvsp[-2].number), ss.object.ss.throttle.limit.clients, 0);
      ASSIGN_THROTTLE_CLIENTS((yyvsp[0].number), ss.object.ss.throttle.limit.seconds, 0);
      ss.object.ss.throttle_isset = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 5908 "config_parse.c"
    break;

  case 409: /* sessionstate_key: SESSIONSTATE_KEY ':' STATEKEY  */
#line 2928 "config_parse.y"
                                                {
#if !SOCKS_CLIENT
      if ((ss.keystate.key = string2statekey((yyvsp[0].string))) == key_unset)
         yyerrorx("%s is not a valid state key", (yyvsp[0].string));

      if (ss.keystate.key == key_hostid) {
#if HAVE_SOCKS_HOSTID

         *hostidoption_isset           = 1;
         ss.keystate.keyinfo.hostindex = DEFAULT_HOSTINDEX;

#else /* !HAVE_SOCKS_HOSTID */

         yyerrorx("hostid is not supported on this system");

#endif /* HAVE_SOCKS_HOSTID */
      }




#else /* SOCKS_CLIENT */

   SERRX(0);
#endif /* SOCKS_CLIENT */
   }
#line 5939 "config_parse.c"
    break;

  case 410: /* $@24: %empty  */
#line 2956 "config_parse.y"
                                           {
#if !SOCKS_CLIENT && HAVE_SOCKS_HOSTID
      hostindex = &ss.keystate.keyinfo.hostindex;
   }
#line 5948 "config_parse.c"
    break;

  case 411: /* sessionstate_keyinfo: SESSIONSTATE_KEY '.' $@24 hostindex  */
#line 2960 "config_parse.y"
             {
      hostindex = &rule.hostindex; /* reset */
#endif /* !SOCKS_CLIENT && HAVE_SOCKS_HOSTID */
   }
#line 5957 "config_parse.c"
    break;

  case 412: /* sessionstate_max: SESSIONSTATE_MAX ':' NUMBER  */
#line 2967 "config_parse.y"
                                              {
#if !SOCKS_CLIENT
      ASSIGN_MAXSESSIONS((yyvsp[0].number), ss.object.ss.max_perstate, 0);
      ss.object.ss.max_perstate_isset = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 5968 "config_parse.c"
    break;

  case 413: /* sessionstate_throttle: SESSIONSTATE_THROTTLE ':' NUMBER '/' NUMBER  */
#line 2975 "config_parse.y"
                                                                   {
#if !SOCKS_CLIENT
   ASSIGN_THROTTLE_SECONDS((yyvsp[-2].number), ss.object.ss.throttle_perstate.limit.clients, 0);
   ASSIGN_THROTTLE_CLIENTS((yyvsp[0].number), ss.object.ss.throttle_perstate.limit.seconds, 0);
   ss.object.ss.throttle_perstate_isset = 1;
#endif /* !SOCKS_CLIENT */
}
#line 5980 "config_parse.c"
    break;

  case 414: /* bandwidth: BANDWIDTH ':' NUMBER  */
#line 2984 "config_parse.y"
                                  {
#if !SOCKS_CLIENT
      ASSIGN_NUMBER((yyvsp[0].number), >=, 0, bw.object.bw.maxbps, 0);
      bw.object.bw.maxbps_isset = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 5991 "config_parse.c"
    break;

  case 416: /* logname: RULE_LOG_CONNECT  */
#line 2996 "config_parse.y"
                           {
#if !SOCKS_CLIENT
         rule.log.connect = 1;
   }
#line 6000 "config_parse.c"
    break;

  case 417: /* logname: RULE_LOG_DATA  */
#line 3000 "config_parse.y"
                     {
         rule.log.data = 1;
   }
#line 6008 "config_parse.c"
    break;

  case 418: /* logname: RULE_LOG_DISCONNECT  */
#line 3003 "config_parse.y"
                           {
         rule.log.disconnect = 1;
   }
#line 6016 "config_parse.c"
    break;

  case 419: /* logname: RULE_LOG_ERROR  */
#line 3006 "config_parse.y"
                      {
         rule.log.error = 1;
   }
#line 6024 "config_parse.c"
    break;

  case 420: /* logname: RULE_LOG_IOOPERATION  */
#line 3009 "config_parse.y"
                            {
         rule.log.iooperation = 1;
   }
#line 6032 "config_parse.c"
    break;

  case 421: /* logname: RULE_LOG_TCPINFO  */
#line 3012 "config_parse.y"
                        {
         rule.log.tcpinfo = 1;
#endif /* !SOCKS_CLIENT */
   }
#line 6041 "config_parse.c"
    break;

  case 424: /* pamservicename: PAMSERVICENAME ':' SERVICENAME  */
#line 3023 "config_parse.y"
                                               {
#if HAVE_PAM && (!SOCKS_CLIENT)
      STRCPY_CHECKLEN(state->pamservicename,
                      (yyvsp[0].string),
                      sizeof(state->pamservicename) -1,
                      yyerrorx);
#else
      yyerrorx_nolib("PAM");
#endif /* HAVE_PAM && (!SOCKS_CLIENT) */
   }
#line 6056 "config_parse.c"
    break;

  case 425: /* bsdauthstylename: BSDAUTHSTYLE ':' BSDAUTHSTYLENAME  */
#line 3035 "config_parse.y"
                                                    {
#if HAVE_BSDAUTH && SOCKS_SERVER
      STRCPY_CHECKLEN(state->bsdauthstylename,
                      (yyvsp[0].string),
                      sizeof(state->bsdauthstylename) - 1,
                      yyerrorx);
#else
      yyerrorx_nolib("bsdauth");
#endif /* HAVE_BSDAUTH && SOCKS_SERVER */
   }
#line 6071 "config_parse.c"
    break;

  case 426: /* gssapiservicename: GSSAPISERVICE ':' GSSAPISERVICENAME  */
#line 3048 "config_parse.y"
                                                       {
#if HAVE_GSSAPI
      STRCPY_CHECKLEN(gssapiservicename,
                      (yyvsp[0].string),
                      sizeof(state->gssapiservicename) - 1,
                      yyerrorx);
#else
      yyerrorx_nolib("GSSAPI");
#endif /* HAVE_GSSAPI */
   }
#line 6086 "config_parse.c"
    break;

  case 427: /* gssapikeytab: GSSAPIKEYTAB ':' GSSAPIKEYTABNAME  */
#line 3060 "config_parse.y"
                                                {
#if HAVE_GSSAPI
#if SOCKS_SERVER
      STRCPY_CHECKLEN(gssapikeytab,
                       (yyvsp[0].string),
                       sizeof(state->gssapikeytab) - 1,
                       yyerrorx);
#else
      yyerrorx("gssapi keytab setting is only applicable to Dante server");
#endif /* SOCKS_SERVER */
#else
      yyerrorx_nolib("GSSAPI");
#endif /* HAVE_GSSAPI */
   }
#line 6105 "config_parse.c"
    break;

  case 429: /* gssapienctypename: GSSAPIENC_ANY  */
#line 3079 "config_parse.y"
                                 {
#if HAVE_GSSAPI
      gssapiencryption->clear           = 1;
      gssapiencryption->integrity       = 1;
      gssapiencryption->confidentiality = 1;
   }
#line 6116 "config_parse.c"
    break;

  case 430: /* gssapienctypename: GSSAPIENC_CLEAR  */
#line 3085 "config_parse.y"
                      {
      gssapiencryption->clear = 1;
   }
#line 6124 "config_parse.c"
    break;

  case 431: /* gssapienctypename: GSSAPIENC_INTEGRITY  */
#line 3088 "config_parse.y"
                          {
      gssapiencryption->integrity = 1;
   }
#line 6132 "config_parse.c"
    break;

  case 432: /* gssapienctypename: GSSAPIENC_CONFIDENTIALITY  */
#line 3091 "config_parse.y"
                                {
      gssapiencryption->confidentiality = 1;
   }
#line 6140 "config_parse.c"
    break;

  case 433: /* gssapienctypename: GSSAPIENC_PERMESSAGE  */
#line 3094 "config_parse.y"
                           {
      yyerrorx("gssapi per-message encryption not supported");
#else
      yyerrorx_nolib("GSSAPI");
#endif /* HAVE_GSSAPI */
   }
#line 6151 "config_parse.c"
    break;

  case 437: /* libwrap: LIBWRAPSTART ':' LINE  */
#line 3109 "config_parse.y"
                                 {
#if HAVE_LIBWRAP && (!SOCKS_CLIENT)
      struct request_info request;
      char tmp[LIBWRAPBUF];
      int errno_s, devnull;

      STRCPY_CHECKLEN(rule.libwrap,
                      (yyvsp[0].string),
                      sizeof(rule.libwrap) - 1,
                      yyerrorx);

      /* libwrap modifies the passed buffer, to test with a tmp one. */
      STRCPY_ASSERTSIZE(tmp, rule.libwrap);

      devnull = open("/dev/null", O_RDWR, 0);
      ++dry_run;
      errno_s = errno;

      errno = 0;

      request_init(&request, RQ_FILE, devnull, RQ_DAEMON, __progname, 0);
      if (setjmp(tcpd_buf) != 0)
         yyerror("bad libwrap line");
      process_options(tmp, &request);

      if (errno != 0)
         yywarn("possible libwrap/tcp-wrappers related configuration error");

      --dry_run;
      close(devnull);
      errno = errno_s;

#else
      yyerrorx_nolib("GSSAPI");
#endif /* HAVE_LIBWRAP && (!SOCKS_CLIENT) */

   }
#line 6193 "config_parse.c"
    break;

  case 442: /* rdr_toaddress: rdr_to ':' address  */
#line 3161 "config_parse.y"
                                  {
#if BAREFOOTD
      yyerrorx("redirecting \"to\" an address does not make any sense in %s.  "
               "Instead specify the address you wanted to \"redirect\" "
               "data to as the \"bounce to\" address, as normal",
               PRODUCT);
#endif /* BAREFOOT */
   }
#line 6206 "config_parse.c"
    break;

  case 454: /* routeoption: socketoption  */
#line 3184 "config_parse.y"
                          {
               if (!addedsocketoption(&route.socketoptionc,
                                      &route.socketoptionv,
                                      &socketopt))
                  yywarn("could not add socketoption");
   }
#line 6217 "config_parse.c"
    break;

  case 455: /* routeoptions: %empty  */
#line 3192 "config_parse.y"
               { (yyval.string) = NULL; }
#line 6223 "config_parse.c"
    break;

  case 458: /* from: FROM  */
#line 3199 "config_parse.y"
             {
      addrinit(&src, 1);
   }
#line 6231 "config_parse.c"
    break;

  case 459: /* to: TO  */
#line 3204 "config_parse.y"
         {
      addrinit(&dst, ipaddr_requires_netmask(to, objecttype));
   }
#line 6239 "config_parse.c"
    break;

  case 460: /* rdr_from: FROM  */
#line 3209 "config_parse.y"
                 {
      addrinit(&rdr_from, 1);
   }
#line 6247 "config_parse.c"
    break;

  case 461: /* rdr_to: TO  */
#line 3214 "config_parse.y"
             {
      addrinit(&rdr_to, 0);
   }
#line 6255 "config_parse.c"
    break;

  case 462: /* bounceto: TO  */
#line 3219 "config_parse.y"
               {
#if BAREFOOTD
      addrinit(&bounceto, 0);
#endif /* BAREFOOTD */
   }
#line 6265 "config_parse.c"
    break;

  case 463: /* via: VIA  */
#line 3227 "config_parse.y"
           {
      gwaddrinit(&gw);
   }
#line 6273 "config_parse.c"
    break;

  case 472: /* ipaddress: ipv4 '/' netmask_v4  */
#line 3247 "config_parse.y"
                               { if (!netmask_required) yyerrorx_hasnetmask(); }
#line 6279 "config_parse.c"
    break;

  case 473: /* ipaddress: ipv4  */
#line 3248 "config_parse.y"
                               { if (netmask_required)  yyerrorx_nonetmask();  }
#line 6285 "config_parse.c"
    break;

  case 474: /* ipaddress: ipv6 '/' netmask_v6  */
#line 3249 "config_parse.y"
                               { if (!netmask_required) yyerrorx_hasnetmask(); }
#line 6291 "config_parse.c"
    break;

  case 475: /* ipaddress: ipv6  */
#line 3250 "config_parse.y"
                               { if (netmask_required)  yyerrorx_nonetmask();  }
#line 6297 "config_parse.c"
    break;

  case 476: /* ipaddress: ipvany '/' netmask_vany  */
#line 3251 "config_parse.y"
                                   { if (!netmask_required)
                                       yyerrorx_hasnetmask(); }
#line 6304 "config_parse.c"
    break;

  case 477: /* ipaddress: ipvany  */
#line 3253 "config_parse.y"
                               { if (netmask_required)  yyerrorx_nonetmask();  }
#line 6310 "config_parse.c"
    break;

  case 480: /* gwaddress: ifname  */
#line 3257 "config_parse.y"
                    { /* for upnp; broadcasts on interface. */ }
#line 6316 "config_parse.c"
    break;

  case 484: /* ipv4: IPV4  */
#line 3266 "config_parse.y"
             {
      *atype = SOCKS_ADDR_IPV4;

      if (socks_inet_pton(AF_INET, (yyvsp[0].string), ipv4, NULL) != 1)
         yyerror("bad %s: %s", atype2string(*atype), (yyvsp[0].string));
   }
#line 6327 "config_parse.c"
    break;

  case 485: /* netmask_v4: NUMBER  */
#line 3274 "config_parse.y"
                     {
      if ((yyvsp[0].number) < 0 || (yyvsp[0].number) > 32)
         yyerrorx("bad %s netmask: %ld.  Legal range is 0 - 32",
                  atype2string(*atype), (long)(yyvsp[0].number));

      netmask_v4->s_addr = (yyvsp[0].number) == 0 ? 0 : htonl(IPV4_FULLNETMASK << (32 - (yyvsp[0].number)));
   }
#line 6339 "config_parse.c"
    break;

  case 486: /* netmask_v4: IPV4  */
#line 3281 "config_parse.y"
                   {
      if (socks_inet_pton(AF_INET, (yyvsp[0].string), netmask_v4, NULL) != 1)
         yyerror("bad %s netmask: %s", atype2string(*atype), (yyvsp[0].string));
   }
#line 6348 "config_parse.c"
    break;

  case 487: /* ipv6: IPV6  */
#line 3287 "config_parse.y"
             {
      *atype = SOCKS_ADDR_IPV6;

      if (socks_inet_pton(AF_INET6, (yyvsp[0].string), ipv6, scopeid_v6) != 1)
         yyerror("bad %s: %s", atype2string(*atype), (yyvsp[0].string));
   }
#line 6359 "config_parse.c"
    break;

  case 488: /* netmask_v6: NUMBER  */
#line 3295 "config_parse.y"
                     {
      if ((yyvsp[0].number) < 0 || (yyvsp[0].number) > IPV6_NETMASKBITS)
         yyerrorx("bad %s netmask: %d.  Legal range is 0 - %d",
                  atype2string(*atype), (int)(yyvsp[0].number), IPV6_NETMASKBITS);

      *netmask_v6 = (yyvsp[0].number);
   }
#line 6371 "config_parse.c"
    break;

  case 489: /* ipvany: IPVANY  */
#line 3304 "config_parse.y"
                 {
      SASSERTX(strcmp((yyvsp[0].string), "0") == 0);

      *atype = SOCKS_ADDR_IPVANY;
      ipvany->s_addr = htonl(0);
   }
#line 6382 "config_parse.c"
    break;

  case 490: /* netmask_vany: NUMBER  */
#line 3312 "config_parse.y"
                       {
      if ((yyvsp[0].number) != 0)
         yyerrorx("bad %s netmask: %d.  Only legal value is 0",
                  atype2string(*atype), (int)(yyvsp[0].number));

      netmask_vany->s_addr = htonl((yyvsp[0].number));
   }
#line 6394 "config_parse.c"
    break;

  case 491: /* domain: DOMAINNAME  */
#line 3322 "config_parse.y"
                     {
      *atype = SOCKS_ADDR_DOMAIN;
      STRCPY_CHECKLEN(domain, (yyvsp[0].string), MAXHOSTNAMELEN - 1, yyerrorx);
   }
#line 6403 "config_parse.c"
    break;

  case 492: /* ifname: IFNAME  */
#line 3328 "config_parse.y"
                 {
      *atype = SOCKS_ADDR_IFNAME;
      STRCPY_CHECKLEN(ifname, (yyvsp[0].string), MAXIFNAMELEN - 1, yyerrorx);
   }
#line 6412 "config_parse.c"
    break;

  case 493: /* url: URL  */
#line 3335 "config_parse.y"
           {
      *atype = SOCKS_ADDR_URL;
      STRCPY_CHECKLEN(url, (yyvsp[0].string), MAXURLLEN - 1, yyerrorx);
   }
#line 6421 "config_parse.c"
    break;

  case 494: /* port: %empty  */
#line 3342 "config_parse.y"
      { (yyval.number) = 0; }
#line 6427 "config_parse.c"
    break;

  case 498: /* gwport: %empty  */
#line 3348 "config_parse.y"
        { (yyval.number) = 0; }
#line 6433 "config_parse.c"
    break;

  case 502: /* portrange: portstart '-' portend  */
#line 3356 "config_parse.y"
                                   {
   if (ntohs(*port_tcp) > ntohs(ruleaddr->portend))
      yyerrorx("end port (%u) can not be less than start port (%u)",
      ntohs(*port_tcp), ntohs(ruleaddr->portend));
   }
#line 6443 "config_parse.c"
    break;

  case 503: /* portstart: NUMBER  */
#line 3364 "config_parse.y"
                    {
      ASSIGN_PORTNUMBER((yyvsp[0].number), *port_tcp);
      ASSIGN_PORTNUMBER((yyvsp[0].number), *port_udp);
   }
#line 6452 "config_parse.c"
    break;

  case 504: /* portend: NUMBER  */
#line 3370 "config_parse.y"
                  {
      ASSIGN_PORTNUMBER((yyvsp[0].number), ruleaddr->portend);
      ruleaddr->operator   = range;
   }
#line 6461 "config_parse.c"
    break;

  case 505: /* portservice: SERVICENAME  */
#line 3376 "config_parse.y"
                           {
      struct servent   *service;

      if ((service = getservbyname((yyvsp[0].string), "tcp")) == NULL) {
         if (state->protocol.tcp)
            yyerrorx("unknown tcp protocol: %s", (yyvsp[0].string));

         *port_tcp = htons(0);
      }
      else
         *port_tcp = (in_port_t)service->s_port;

      if ((service = getservbyname((yyvsp[0].string), "udp")) == NULL) {
         if (state->protocol.udp)
               yyerrorx("unknown udp protocol: %s", (yyvsp[0].string));

            *port_udp = htons(0);
      }
      else
         *port_udp = (in_port_t)service->s_port;

      if (*port_tcp == htons(0) && *port_udp == htons(0))
         yyerrorx("unknown tcp/udp protocol");

      /* if one protocol is unset, set to same as the other. */
      if (*port_tcp == htons(0))
         *port_tcp = *port_udp;
      else if (*port_udp == htons(0))
         *port_udp = *port_tcp;

      (yyval.number) = (size_t)*port_udp;
   }
#line 6498 "config_parse.c"
    break;

  case 506: /* portoperator: OPERATOR  */
#line 3411 "config_parse.y"
                         {
      *operator = string2operator((yyvsp[0].string));
   }
#line 6506 "config_parse.c"
    break;

  case 508: /* udpportrange_start: NUMBER  */
#line 3420 "config_parse.y"
                           {
#if SOCKS_SERVER
   ASSIGN_PORTNUMBER((yyvsp[0].number), rule.udprange.start);
#endif /* SOCKS_SERVER */
   }
#line 6516 "config_parse.c"
    break;

  case 509: /* udpportrange_end: NUMBER  */
#line 3427 "config_parse.y"
                         {
#if SOCKS_SERVER
   ASSIGN_PORTNUMBER((yyvsp[0].number), rule.udprange.end);
   rule.udprange.op  = range;

   if (ntohs(rule.udprange.start) > ntohs(rule.udprange.end))
      yyerrorx("end port (%d) can not be less than start port (%u)",
               (int)(yyvsp[0].number), ntohs(rule.udprange.start));
#endif /* SOCKS_SERVER */
   }
#line 6531 "config_parse.c"
    break;

  case 510: /* number: NUMBER  */
#line 3439 "config_parse.y"
               {
      addnumber(&numberc, &numberv, (yyvsp[0].number));
   }
#line 6539 "config_parse.c"
    break;


#line 6543 "config_parse.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 3449 "config_parse.y"


#define INTERACTIVE      0

extern FILE *yyin;

int lex_dorestart; /* global for Lex. */

int
parseconfig(filename)
   const char *filename;
{
   struct stat statbuf;
   int haveconfig;

#if SOCKS_CLIENT /* assume server admin can set things up correctly himself. */
   parseclientenv(&haveconfig);

   if (haveconfig)
      return 0;

#else /* !SOCKS_CLIENT */
   SASSERTX(pidismainmother(sockscf.state.pid));

   if (sockscf.state.inited)
      /* in case we need something special to (re)open config-file. */
      sockd_priv(SOCKD_PRIV_PRIVILEGED, PRIV_ON);
#endif /* !SOCKS_CLIENT */

   yyin = fopen(filename, "r");

#if !SOCKS_CLIENT
   if (sockscf.state.inited)
      sockd_priv(SOCKD_PRIV_PRIVILEGED, PRIV_OFF);
#endif /* SERVER */

   if (yyin == NULL
   ||  (stat(filename, &statbuf) == 0 && statbuf.st_size == 0)) {
      if (yyin == NULL)
         slog(sockscf.state.inited ? LOG_WARNING : LOG_ERR,
              "%s: could not open config file %s", function, filename);
      else
         slog((sockscf.state.inited || SOCKS_CLIENT) ? LOG_WARNING : LOG_ERR,
              "%s: config file %s is empty.  Not parsing", function, filename);

#if SOCKS_CLIENT

      if (yyin == NULL) {
         if (sockscf.option.directfallback)
            slog(LOG_DEBUG,
                 "%s: no %s, but direct fallback enabled, continuing",
                 function, filename);
         else
            exit(0);
      }
      else {
         slog(LOG_DEBUG, "%s: empty %s, assuming direct fallback wanted",
              function, filename);

         sockscf.option.directfallback = 1;
      }

      SASSERTX(sockscf.option.directfallback == 1);
#else /* !SOCKS_CLIENT */

      if (!sockscf.state.inited)
         sockdexit(EXIT_FAILURE);

      /*
       * Might possibly continue with old config.
       */

#endif /* !SOCKS_CLIENT */

      haveconfig = 0;
   }
   else {
#if YYDEBUG
      yydebug       = 0;
#endif /* YYDEBUG */

      yylineno      = 1;
      errno         = 0;   /* don't report old errors in yyparse(). */
      haveconfig    = 1;

      /*
       * Special and delayed as long as we can, till immediately before
       * parsing new config.
       * Want to keep a backup of old ones until we know there were no
       * errors adding new logfiles.
       */

#if !SOCKS_CLIENT
      logformat_seen       = 0;
      old_log              = sockscf.log;
      old_errlog           = sockscf.errlog;
#endif /* !SOCKS_CLIENT */

      failed_to_add_errlog = failed_to_add_log = 0;

      slog(LOG_DEBUG, "%s: parsing config in file %s", function, filename);

      bzero(&sockscf.log,    sizeof(sockscf.log));
      bzero(&sockscf.errlog, sizeof(sockscf.errlog));

      lex_dorestart = 1;

      parsingconfig = 1;

#if SOCKSLIBRARY_DYNAMIC
      socks_markasnative("*");
#endif /* SOCKSLIBRARY_DYNAMIC */

      if (yyparse() == 0) {
#if !SOCKS_CLIENT
         /* Keep the previous format until the new config has been parsed. */
         if (!logformat_seen)
            sockscf.logformat = LOGFORMAT_RAW;
#endif /* !SOCKS_CLIENT */
      }

#if SOCKSLIBRARY_DYNAMIC
      socks_markasnormal("*");
#endif /* SOCKSLIBRARY_DYNAMIC */

      parsingconfig = 0;

#if !SOCKS_CLIENT
      CMDLINE_OVERRIDE(&sockscf.initial.cmdline, &sockscf.option);

#if !HAVE_PRIVILEGES
      if (!sockscf.state.inited) {
         /*
          * first time.
          */
         if (sockscf.uid.privileged_isset && !sockscf.option.verifyonly) {
            /*
             * If we created any logfiles (rather than just opened already
             * existing ones), they will have been created with the euid/egid
             * we are started with.  If logfiles created by that euid/egid are
             * not writable by our configured privileged userid (if any), it
             * means that upon SIGHUP we will be unable to re-open our own
             * logfiles.  We therefor check whether the logfile(s) were created
             * by ourselves, and if so, make sure they have the right owner.
             */
            logtype_t *logv[] = { &sockscf.log, &sockscf.errlog };
            size_t i;

            for (i = 0; i < ELEMENTS(logv); ++i) {
               size_t fi;

               for (fi = 0; fi < logv[i]->filenoc; ++fi) {
                  if (logv[i]->createdv[fi]) {
                     slog(LOG_DEBUG,
                          "%s: chown(2)-ing created logfile %s to %lu/%lu",
                          function,
                          logv[i]->fnamev[fi],
                          (unsigned long)sockscf.uid.privileged_uid,
                          (unsigned long)sockscf.uid.privileged_gid);

                     if (fchown(logv[i]->filenov[fi],
                                (unsigned long)sockscf.uid.privileged_uid,
                                (unsigned long)sockscf.uid.privileged_gid) != 0)
                        serr("%s: could not fchown(2) created logfile %s to "
                             "privileged uid/gid %lu/%lu.  This means that "
                             "upon SIGHUP, we would not be unable to re-open "
                             "our own logfiles.  This should not happen",
                             function,
                             logv[i]->fnamev[fi],
                             (unsigned long)sockscf.uid.privileged_uid,
                             (unsigned long)sockscf.uid.privileged_gid);
                  }
               }
            }
         }
      }
#endif /* !HAVE_PRIVILEGES */

      if (configure_privileges() != 0) {
         if (sockscf.state.inited) {
            swarn("%s: could not reinitialize privileges after SIGHUP.  "
                  "Will continue without privileges",
                  function);

            sockscf.state.haveprivs = 0;
         }
         else
            serr("%s: could not configure privileges", function);
      }
#endif /* !SOCKS_CLIENT */
   }

   if (yyin != NULL)
      fclose(yyin);

   errno = 0;
   return haveconfig ? 0 : -1;
}

static int
ipaddr_requires_netmask(context, type)
   const addresscontext_t context;
   const objecttype_t type;
{

   switch (type) {
      case object_crule:
#if HAVE_SOCKS_RULES

         return 1;

#else /* !HAVE_SOCKS_RULES */

         switch (context) {
            case from:
               return 1;

            case to:
               return 0; /* address we accept clients on. */

            case bounce:
               return 0; /* address we connect to.        */

            default:
               SERRX(context);
         }
#endif /* !HAVE_SOCKS_RULES */


#if HAVE_SOCKS_HOSTID
      case object_hrule:
         return 1;
#endif /* HAVE_SOCKS_HOSTID */

#if HAVE_SOCKS_RULES
      case object_srule:
         return 1;
#endif /* HAVE_SOCKS_RULES */

      case object_route:
      case object_monitor:
         return 1;

      default:
         SERRX(type);
   }


   /* NOTREACHED */
   return 0;
}


static void
addnumber(nc, nv, number)
   size_t *nc;
   long long *nv[];
   const long long number;
{
   const char *_function = "addnumber()";

   if ((*nv = realloc(*nv, sizeof(**nv) * ((*nc) + 1)))
   == NULL)
      yyerror("%s: could not allocate %lu bytes of memory for adding "
              "number %lld",
              _function, (unsigned long)(sizeof(**nv) * ((*nc) + 1)),
              number);

   (*nv)[(*nc)++] = number;
}


static void
addrinit(addr, _netmask_required)
   ruleaddr_t *addr;
   const int _netmask_required;
{

   atype            = &addr->atype;

   ipv4             = &addr->addr.ipv4.ip;
   netmask_v4       = &addr->addr.ipv4.mask;

   ipv6             = &addr->addr.ipv6.ip;
   netmask_v6       = &addr->addr.ipv6.maskbits;
   scopeid_v6       = &addr->addr.ipv6.scopeid;

   ipvany           = &addr->addr.ipvany.ip;
   netmask_vany     = &addr->addr.ipvany.mask;

   if (!_netmask_required) {
      netmask_v4->s_addr   = htonl(IPV4_FULLNETMASK);
      *netmask_v6          = IPV6_NETMASKBITS;
      netmask_vany->s_addr = htonl(IPV4_FULLNETMASK);
   }

   domain           = addr->addr.domain;
   ifname           = addr->addr.ifname;

   port_tcp         = &addr->port.tcp;
   port_udp         = &addr->port.udp;
   operator         = &addr->operator;

   netmask_required = _netmask_required;
   ruleaddr         = addr;
}

static void
gwaddrinit(addr)
   sockshost_t *addr;
{
   static enum operator_t operatormem;

   netmask_required = 0;

   atype            = &addr->atype;

   ipv4             = &addr->addr.ipv4;
   ipv6             = &addr->addr.ipv6.ip;
   domain           = addr->addr.domain;
   ifname           = addr->addr.ifname;
   url              = addr->addr.urlname;

   port_tcp         = &addr->port;
   port_udp         = &addr->port;
   operator         = &operatormem; /* no operator in gwaddr and not used. */
}

static void
routeinit(r)
   route_t *r;
{
   bzero(r, sizeof(*r));

   state               = &r->gw.state;
   extension           = &state->extension;

   cmethodv            = state->cmethodv;
   cmethodc            = &state->cmethodc;
   smethodv            = state->smethodv;
   smethodc            = &state->smethodc;

#if HAVE_GSSAPI
   gssapiservicename = state->gssapiservicename;
   gssapikeytab      = state->gssapikeytab;
   gssapiencryption  = &state->gssapiencryption;
#endif /* HAVE_GSSAPI */

#if !SOCKS_CLIENT && HAVE_LDAP
   ldapauthorisation              = &state->ldapauthorisation;
   ldapauthentication          = &state->ldapauthentication;
#endif /* !SOCKS_CLIENT && HAVE_LDAP*/

   bzero(&src, sizeof(src));
   bzero(&dst, sizeof(dst));
   src.atype = SOCKS_ADDR_IPV4;
   dst.atype = SOCKS_ADDR_IPV4;

   bzero(&gw, sizeof(gw));
   bzero(&rdr_from, sizeof(rdr_from));
   bzero(&hostid, sizeof(hostid));
}


#if SOCKS_CLIENT
static void
parseclientenv(haveproxyserver)
   int *haveproxyserver;
{
   const char *function = "parseclientenv()";
   const char *fprintf_error = "could not write to tmpfile used to hold "
                               "settings set in environment for parsing";
   const char *p;
   size_t i;
   FILE *fp;
   char rdr_from[512], extrarouteinfo[sizeof(rdr_from) + sizeof("\n")],
        gw[MAXSOCKSHOSTLEN + sizeof(" port = 65535")];
   int fd;


#if 1

#if SOCKS_CLIENT
   p = "yaccenv-client-XXXXXX";
#else /* !SOCKS_CLIENT */
   p = "yaccenv-server-XXXXXX";
#endif /* !SOCKS_CLIENT */

   if ((fd = socks_mklock(p, NULL, 0)) == -1)
      yyerror("socks_mklock() failed to create tmpfile using base %s", p);

#else /* for debugging file-generation problems. */
   if ((fd = open("/tmp/dante-envfile",
                  O_CREAT | O_TRUNC | O_RDWR,
                  S_IRUSR | S_IWUSR)) == -1)
      serr("%s: could not open file", function);
#endif

   if ((fp = fdopen(fd, "r+")) == NULL)
      serr("%s: fdopen(fd %d) failed", function, fd);

   if ((p = socks_getenv(ENV_SOCKS_LOGOUTPUT, dontcare)) != NULL && *p != NUL)
      if (fprintf(fp, "logoutput: %s\n", p) == -1)
         serr("%s: %s", function, fprintf_error);

   if ((p = socks_getenv(ENV_SOCKS_ERRLOGOUTPUT, dontcare)) != NULL
   && *p != NUL)
      if (fprintf(fp, "errorlog: %s\n", p) == -1)
         serr("%s: %s", function, fprintf_error);

   if ((p = socks_getenv(ENV_SOCKS_DEBUG, dontcare)) != NULL && *p != NUL)
      if (fprintf(fp, "debug: %s\n", p) == -1)
         serr("%s: %s", function, fprintf_error);

   *rdr_from = NUL;
   if ((p = socks_getenv(ENV_SOCKS_REDIRECT_FROM, dontcare)) != NULL
   && *p != NUL) {
      const char *prefix = "redirect from";

      if (strlen(prefix) + strlen(p) + 1 > sizeof(rdr_from))
         serr("%s: %s value is too long.  Max length is %lu",
              function,
              ENV_SOCKS_REDIRECT_FROM,
              (unsigned long)sizeof(rdr_from) - (strlen(prefix) + 1));

      snprintf(rdr_from, sizeof(rdr_from), "%s: %s\n", prefix, p);
   }

   snprintf(extrarouteinfo, sizeof(extrarouteinfo),
            "%s", rdr_from);

   /*
    * Check if there is a proxy server configured in the environment.
    * Initially assume there is none.
    */

   *haveproxyserver = 0;

   i = 1;
   while (1) {
      /* 640 routes should be enough for anyone. */
      char name[sizeof(ENV_SOCKS_ROUTE_) + sizeof("640")];

      snprintf(name, sizeof(name), "%s%lu", ENV_SOCKS_ROUTE_, (unsigned long)i);

      if ((p = socks_getenv(name, dontcare)) == NULL)
         break;

      if (*p != NUL) {
         if (fprintf(fp, "route { %s }\n", p) == -1)
            serr("%s: %s", function, fprintf_error);

         *haveproxyserver = 1;
      }

      ++i;
   }

   if ((p = socks_getenv(ENV_SOCKS4_SERVER, dontcare)) != NULL && *p != NUL) {
      if (fprintf(fp,
"route {\n"
"         from: 0.0.0.0/0 to: 0.0.0.0/0 via: %s\n"
"         proxyprotocol: socks_v4\n"
"         %s"
"}\n",            serverstring2gwstring(p, PROXY_SOCKS_V4, gw, sizeof(gw)),
                  extrarouteinfo) == -1)
         serr("%s: %s", function, fprintf_error);

      *haveproxyserver = 1;
   }

   if ((p = socks_getenv(ENV_SOCKS5_SERVER, dontcare)) != NULL && *p != NUL) {
      if (fprintf(fp,
"route {\n"
"         from: 0.0.0.0/0 to: 0.0.0.0/0 via: %s\n"
"         proxyprotocol: socks_v5\n"
"         %s"
"}\n",            serverstring2gwstring(p, PROXY_SOCKS_V5, gw, sizeof(gw)),
                  extrarouteinfo) == -1)
         serr("%s: %s", function, fprintf_error);

      *haveproxyserver = 1;
   }

   if ((p = socks_getenv(ENV_SOCKS_SERVER, dontcare)) != NULL && *p != NUL) {
      if (fprintf(fp,
"route {\n"
"         from: 0.0.0.0/0 to: 0.0.0.0/0 via: %s\n"
"         %s"
"}\n",            serverstring2gwstring(p, PROXY_SOCKS_V5, gw, sizeof(gw)),
                  extrarouteinfo) == -1)
         serr("%s: %s", function, fprintf_error);

      *haveproxyserver = 1;
   }

   if ((p = socks_getenv(ENV_HTTP_PROXY, dontcare)) != NULL && *p != NUL) {
      struct sockaddr_storage sa;
      int gaierr;
      char emsg[512];

      if (urlstring2sockaddr(p, &sa, &gaierr, emsg, sizeof(emsg)) == NULL)
         serr("%s: could not convert to %s to an Internet address",
              function, p);

      if (fprintf(fp,
"route {\n"
"         from: 0.0.0.0/0 to: 0.0.0.0/0 via: %s port = %d\n"
"         proxyprotocol: http_v1.0\n"
"         %s"
"}\n",
                  sockaddr2string2(&sa, 0, NULL, 0),
                  ntohs(GET_SOCKADDRPORT(&sa)),
                  extrarouteinfo)
      == -1)
         serr("%s: %s", function, fprintf_error);

      *haveproxyserver = 1;
   }

   if ((p = socks_getenv(ENV_UPNP_IGD, dontcare)) != NULL && *p != NUL) {
      if (fprintf(fp,
"route {\n"
"         from: 0.0.0.0/0 to: 0.0.0.0/0 via: %s\n"
"         proxyprotocol: upnp\n"
"         %s"
"}\n",            p, extrarouteinfo) == -1)
         serr("%s: %s", function, fprintf_error);

      *haveproxyserver = 1;
   }


   /*
    * End of possible settings we want to parse with yacc/lex.
    */

   if (fseek(fp, 0, SEEK_SET) != 0)
      yyerror("fseek(3) on tmpfile used to hold environment-settings failed");

   yyin = fp;

   lex_dorestart             = 1;
   parsingconfig             = 1;
   p                         = sockscf.option.configfile;
   sockscf.option.configfile = "<generated socks.conf>";

#if SOCKSLIBRARY_DYNAMIC
   socks_markasnative("*");
#endif /* SOCKSLIBRARY_DYNAMIC */

   yyparse();

#if SOCKSLIBRARY_DYNAMIC
   socks_markasnormal("*");
#endif /* SOCKSLIBRARY_DYNAMIC */

   sockscf.option.configfile = p;
   parsingconfig             = 0;

   fclose(fp);

   if (socks_getenv(ENV_SOCKS_AUTOADD_LANROUTES, isfalse) == NULL) {
      /*
       * assume it's good to add direct routes for the lan also.
       */
      struct ifaddrs *ifap;

      slog(LOG_DEBUG, "%s: auto-adding direct routes for lan ...", function);

      if (getifaddrs(&ifap) == 0) {
         command_t commands;
         protocol_t protocols;
         struct ifaddrs *iface;

         bzero(&commands, sizeof(commands));
         bzero(&protocols, sizeof(protocols));

         protocols.tcp = 1;
         protocols.udp = 1;

         commands.connect      = 1;
         commands.udpassociate = 1;

         for (iface = ifap; iface != NULL; iface = iface->ifa_next)
            if (iface->ifa_addr            != NULL
            &&  iface->ifa_addr->sa_family == AF_INET) {
               if (iface->ifa_netmask == NULL) {
                  swarn("interface %s missing netmask, skipping",
                        iface->ifa_name);
                  continue;
               }

               socks_autoadd_directroute(&commands,
                                         &protocols,
                                         TOCSS(iface->ifa_addr),
                                         TOCSS(iface->ifa_netmask));
            }

         freeifaddrs(ifap);
      }
   }
   else
      slog(LOG_DEBUG, "%s: not auto-adding direct routes for lan", function);
}

static char *
serverstring2gwstring(serverstring, version, gw, gwsize)
   const char *serverstring;
   const int version;
   char *gw;
   const size_t gwsize;
{
   const char *function = "serverstring2gwstring()";
   char *sep, emsg[256];

   if (version != PROXY_SOCKS_V4 && version != PROXY_SOCKS_V5)
      return gw; /* should be in desired format already. */

   if (strlen(serverstring) >= gwsize)
      serrx("%s: value of proxyserver (%s) set in environment is too long.  "
            "Max length is %lu",
            function, serverstring, (unsigned long)(gwsize - 1));

   if ((sep = strrchr(serverstring, ':')) != NULL && *(sep + 1) != NUL) {
      long port;

      if ((port = string2portnumber(sep + 1, emsg, sizeof(emsg))) == -1)
         yyerrorx("%s: %s", function, emsg);

      memcpy(gw, serverstring, sep - serverstring);
      snprintf(&gw[sep - serverstring],
               gwsize - (sep - serverstring),
               " port = %u",
               (in_port_t)port);
   }
   else {
      char visbuf[256];

      yyerrorx("%s: could not find portnumber in %s serverstring \"%s\"",
               function,
               proxyprotocol2string(version),
               str2vis(sep == NULL ? serverstring : sep,
                       strlen(sep == NULL ? serverstring : sep),
                       visbuf,
                       sizeof(visbuf)));
   }

   return gw;
}

#else /* !SOCKS_CLIENT */

static void
pre_addrule(rule)
   rule_t *rule;
{

   rule->src   = src;
   rule->dst   = dst;

#if HAVE_SOCKS_HOSTID
   rule->hostid      = hostid;
#endif /* HAVE_SOCKS_HOSTID */

   rule->rdr_from    = rdr_from;
   rule->rdr_to      = rdr_to;

   if (session_isset) {
      if ((rule->ss = malloc(sizeof(*rule->ss))) == NULL)
         yyerror("failed to malloc(3) %lu bytes for session memory",
                 (unsigned long)sizeof(*rule->ss));

      *rule->ss = ss;
   }

   if (bw_isset) {
      if ((rule->bw = malloc(sizeof(*rule->bw))) == NULL)
         yyerror("failed to malloc(3) %lu bytes for bw memory",
                 (unsigned long)sizeof(*rule->bw));

      *rule->bw = bw;
   }
}


static void
post_addrule(void)
{

   timeout = &sockscf.timeout; /* default is global timeout, unless in a rule */
}

static void
ruleinit(rule)
   rule_t *rule;
{
   bzero(rule, sizeof(*rule));

   rule->linenumber  = yylineno;

#if HAVE_SOCKS_HOSTID

   rule->hostindex          = DEFAULT_HOSTINDEX;
   hostindex                = &rule->hostindex;

   rule->hostidoption_isset = 0;
   hostidoption_isset       = &rule->hostidoption_isset;

#endif /* HAVE_SOCKS_HOSTID */

   state          = &rule->state;

   cmethodv       = state->cmethodv;
   cmethodc       = &state->cmethodc;

   smethodv       = state->smethodv;
   smethodc       = &state->smethodc;

   /*
    * default values: same as global.
    */

   timeout       = &rule->timeout;
   *timeout      = sockscf.timeout;

#if HAVE_GSSAPI

   gssapiservicename = state->gssapiservicename;
   gssapikeytab      = state->gssapikeytab;
   gssapiencryption  = &state->gssapiencryption;

#endif /* HAVE_GSSAPI */

#if HAVE_LDAP

   ldapauthorisation              = &state->ldapauthorisation;
   ldapauthentication             = &state->ldapauthentication;

   /*
    * Common attribute settings, LDAP authentication/authorisation. 
    */

   ldapauthorisation->auto_off    = ldapauthentication->auto_off  = -1;
   ldapauthorisation->certcheck   = ldapauthentication->certcheck = -1;

   ldapauthorisation->debug       = ldapauthentication->debug
   = LDAP_UNSET_DEBUG_VALUE;

   ldapauthorisation->keeprealm   = ldapauthorisation->keeprealm  = -1;
   ldapauthorisation->port        = ldapauthentication->port      = -1;
   ldapauthorisation->portssl     = ldapauthentication->portssl   = -1;
   ldapauthorisation->ssl         = ldapauthentication->ssl       = -1;

   /*
    * Only in LDAP authorisation.
    */
   ldapauthorisation->mdepth                                      = -1;

   /*
    * Rest should be char arrays and NUL already due to bzero(3).
    */

#endif /* HAVE_LDAP */

#if HAVE_PAC

   rule->objectsids  = NULL;
   rule->pacoff      = 1;

#endif /* HAVE_PAC */

   bzero(&src, sizeof(src));
   bzero(&dst, sizeof(dst));
   bzero(&hostid, sizeof(hostid));

   bzero(&rdr_from, sizeof(rdr_from));
   bzero(&rdr_to, sizeof(rdr_to));

#if BAREFOOTD
   bzero(&bounceto, sizeof(bounceto));
#endif /* BAREFOOTD */

   rule->bw_isinheritable   = rule->ss_isinheritable = 1;

   bzero(&ss, sizeof(ss));
   bzero(&bw, sizeof(bw));

   bw_isset = session_isset = 0;
   bw.type  = SHMEM_BW;
   ss.type  = SHMEM_SS;
}

void
alarminit(void)
{
    static int alarmside_mem;

   alarmside  = &alarmside_mem;
   *alarmside = 0;
}

static void
monitorinit(monitor)
   monitor_t *monitor;
{
   static int alarmside_mem;

   alarmside = &alarmside_mem;

   bzero(monitor, sizeof(*monitor));

   monitor->linenumber = yylineno;

   state                       = &monitor->state;

#if HAVE_SOCKS_HOSTID
   monitor->hostindex          = DEFAULT_HOSTINDEX;
   hostindex                   = &monitor->hostindex;

   monitor->hostidoption_isset = 0;
   hostidoption_isset          = &monitor->hostidoption_isset;
#endif /* HAVE_SOCKS_HOSTID */

   bzero(&src, sizeof(src));
   bzero(&dst, sizeof(dst));
   bzero(&hostid, sizeof(hostid));

   if ((monitor->mstats = malloc(sizeof(*monitor->mstats))) == NULL)
      yyerror("failed to malloc(3) %lu bytes for monitor stats memory",
              (unsigned long)sizeof(*monitor->mstats));
   else
      bzero(monitor->mstats, sizeof(*monitor->mstats));

   monitor->mstats->type = SHMEM_MONITOR;
}

static void
pre_addmonitor(monitor)
   monitor_t *monitor;
{
   monitor->src    = src;
   monitor->dst    = dst;

#if HAVE_SOCKS_HOSTID
   monitor->hostid = hostid;
#endif /* HAVE_SOCKS_HOSTID */
}

static int
configure_privileges(void)
{
   const char *function = "configure_privileges()";
   static int isfirsttime = 1;

   if (sockscf.option.verifyonly)
      return 0;

#if !HAVE_PRIVILEGES
   uid_t uid; /* for debugging. */
   gid_t gid; /* for debugging. */

   SASSERTX(sockscf.state.euid == (uid = geteuid()));
   SASSERTX(sockscf.state.egid == (gid = getegid()));

   /*
    * Check all configured uids/gids work.
    */

   checkugid(&sockscf.uid.privileged_uid,
             &sockscf.uid.privileged_gid,
             &sockscf.uid.privileged_isset,
             "privileged");

   checkugid(&sockscf.uid.unprivileged_uid,
             &sockscf.uid.unprivileged_gid,
             &sockscf.uid.unprivileged_isset,
             "unprivileged");

#if HAVE_LIBWRAP
   if (!sockscf.uid.libwrap_isset
   &&  sockscf.uid.unprivileged_isset) {
      sockscf.uid.libwrap_uid   = sockscf.uid.unprivileged_uid;
      sockscf.uid.libwrap_gid   = sockscf.uid.unprivileged_gid;
      sockscf.uid.libwrap_isset = sockscf.uid.unprivileged_isset;
   }
   else
      checkugid(&sockscf.uid.libwrap_uid,
                &sockscf.uid.libwrap_gid,
                &sockscf.uid.libwrap_isset,
                "libwrap");
#endif /* HAVE_LIBWRAP */

   SASSERTX(sockscf.state.euid == (uid = geteuid()));
   SASSERTX(sockscf.state.egid == (gid = getegid()));

#endif /* !HAVE_PRIVILEGES */

   if (isfirsttime) {
      if (sockd_initprivs() != 0) {
         slog(HAVE_PRIVILEGES ? LOG_INFO : LOG_WARNING,
              "%s: could not initialize privileges (%s)%s",
              function,
              strerror(errno),
              geteuid() == 0 ?
                   "" : ".  Usually we need to be started by root if "
                        "special privileges are to be available");

#if HAVE_PRIVILEGES
         /*
          * assume failure in this case is not fatal; some privileges will
          * not be available to us, and perhaps that is the intention too.
          */
         return 0;

#else
         return -1;
#endif /* !HAVE_PRIVILEGES */
      }

      isfirsttime = 0;
   }

   return 0;
}

static int
checkugid(uid, gid, isset, type)
   uid_t *uid;
   gid_t *gid;
   unsigned char *isset;
   const char *type;
{
   const char *function = "checkugid()";

   SASSERTX(sockscf.state.euid == geteuid());
   SASSERTX(sockscf.state.egid == getegid());

   if (sockscf.option.verifyonly)
      return 0;

   if (!(*isset)) {
      *uid   = sockscf.state.euid;
      *gid   = sockscf.state.egid;
      *isset = 1;

      return 0;
   }

   if (*uid != sockscf.state.euid) {
      if (seteuid(*uid) != 0) {
         swarn("%s: could not seteuid(2) to %s uid %lu",
               function, type, (unsigned long)*uid);

         return -1;
      }

      (void)seteuid(0);

      if (seteuid(sockscf.state.euid) != 0) {
         swarn("%s: could not revert to euid %lu from euid %lu",
               function,
               (unsigned long)sockscf.state.euid,
               (unsigned long)geteuid());
         SWARN(0);

         sockscf.state.euid = geteuid();
         return -1;
      }
   }

   if (*gid != sockscf.state.egid) {
      (void)seteuid(0);

      if (setegid(*gid) != 0) {
         swarn("%s: could not setegid(2) to %s gid %lu",
               function, type, (unsigned long)*gid);

         return -1;
      }

      (void)seteuid(0);

      if (setegid(sockscf.state.egid) != 0) {
         swarn("%s: could not revert to egid %lu from euid %lu",
               function,
               (unsigned long)sockscf.state.egid,
               (unsigned long)geteuid());
         SWARN(0);

         sockscf.state.egid = getegid();
         return -1;
      }

      if (seteuid(sockscf.state.euid) != 0) {
         swarn("%s: could not revert to euid %lu from euid %lu",
               function,
               (unsigned long)sockscf.state.euid,
               (unsigned long)geteuid());
         SWARN(0);

         sockscf.state.euid = geteuid();
         return -1;
      }
   }

   SASSERTX(sockscf.state.euid == geteuid());
   SASSERTX(sockscf.state.egid == getegid());

   return 0;
}

#endif /* !SOCKS_CLIENT */
