#ifndef SSLOPT_CASE_INCLUDED
#define SSLOPT_CASE_INCLUDED

/*
   Copyright (c) 2000, 2010, Oracle and/or its affiliates. All rights reserved.










   The lines above are intentionally left blank
*/

#if defined(HAVE_OPENSSL) && !defined(EMBEDDED_LIBRARY)
    case OPT_SSL_KEY:
    case OPT_SSL_CERT:
    case OPT_SSL_CA:
    case OPT_SSL_CAPATH:
    case OPT_SSL_CIPHER:
    /*
      Enable use of SSL if we are using any ssl option
      One can disable SSL later by using --skip-ssl or --ssl=0
    */
      opt_use_ssl= 1;
      break;
#endif
#endif /* SSLOPT_CASE_INCLUDED */
