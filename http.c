/*
 * HTTP notifier for CUPS.
 *
 * Copyright 2017 by Aiwip Ltd.
 *
 * Licensed under Apache License v2.0.  See the file "LICENSE" for more information.
 */

/*
 * Include necessary headers...
 */

#define _IPP_PRIVATE_STRUCTURES 1

#include <curl/curl.h>
#include <cups/cups.h>
#include <signal.h>
#include <stdio.h>
 
 /*
  * Local functions...
  */
 
 void print_attributes(ipp_t *ipp, int indent);
 
 
 /*
  * 'main()' - Read events and send HTTP notifications.
  */
 
 int					                              /* O - Exit status */
 main(int  argc,			                      /* I - Number of command-line arguments */
      char *argv[])			                    /* I - Command-line arguments */
 {
    int		              i;			            /* Looping var */
    char                scheme[32],		      /* URI scheme ("http") */
                        host[1024],		      /* Hostname for remote HTTP  */
                        username[256],		  /* Username for remote HTTP */
                        resource[1024];		  /* HTTP endpoint */
    int		              port;			          /* Port number for remote HTTP */
    struct sigaction	  action;		          /* POSIX sigaction data */
    ipp_t		            *event;			        /* Event from scheduler */
    ipp_state_t	        state;			        /* IPP event state */
    int		              exit_status;		    /* Exit status */
    CURL                *curl;              /* CURL HTTP Client */
    CURLcode            res;                /* HTTP Response */
    char		            baseurl[1024];		  /* Base URL */

    /*
     * Don't buffer stderr...
     */
    setbuf(stderr, NULL);
    
    /*
     * Ignore SIGPIPE signals...
     */

    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &action, NULL);

    fprintf(stderr, "DEBUG: argc=%d\n", argc);
    for (i = 0; i < argc; i ++)
      fprintf(stderr, "DEBUG: argv[%d]=\"%s\"\n", i, argv[i]);
    fprintf(stderr, "DEBUG: TMPDIR=\"%s\"\n", getenv("TMPDIR"));

    /*
     * See whether the HTTP Endpoint is based locally or remotely...
     */

    if (httpSeparateURI(HTTP_URI_CODING_ALL, argv[1], scheme, sizeof(scheme),
            username, sizeof(username), host, sizeof(host), &port,
            resource, sizeof(resource)) < HTTP_URI_OK)
    {
        fprintf(stderr, "ERROR: Bad HTTP URI \"%s\"!\n", argv[1]);
        return (1);
    }

    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, baseurl);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
 
    /*
     * Loop forever until we run out of events...
     */

    for (;;)
    {

        /*
         * Read the next event...
         */
         
        event = ippNew();
        while ((state = ippReadFile(0, event)) != IPP_DATA)
        {
          if (state <= IPP_IDLE)
            break;
        }

        fprintf(stderr, "DEBUG: state=%d\n", state);

        if (state == IPP_ERROR)
            fputs("DEBUG: ippReadFile() returned IPP_ERROR!\n", stderr);

        if (state <= IPP_IDLE)
            break;

        // TODO: Generate body from attributes
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");
        print_attributes(event, 4);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
            exit(0);

        /*
         * Free the memory used for this event...
         */

        curl_easy_cleanup(curl);
        ippDelete(event);
        event = NULL;
    }

    return (0);
 }


/*
 * 'print_attributes()' - Print the attributes in a request...
 */

void
print_attributes(ipp_t *ipp,		/* I - IPP request */
                 int   indent)		/* I - Indentation */
{
  ipp_tag_t		    group;		    /* Current group */
  ipp_attribute_t	*attr;		    /* Current attribute */
  char			    buffer[1024];	/* Value buffer */

  for (group = IPP_TAG_ZERO, attr = ipp->attrs; attr; attr = attr->next)
  {
    if ((attr->group_tag == IPP_TAG_ZERO && indent <= 8) || !attr->name)
    {
      group = IPP_TAG_ZERO;
      fputc('\n', stderr);
      continue;
    }

    if (group != attr->group_tag)
    {
      group = attr->group_tag;

      fprintf(stderr, "DEBUG: %*s%s:\n\n", indent - 4, "", ippTagString(group));
    }

    ippAttributeString(attr, buffer, sizeof(buffer));

    fprintf(stderr, "DEBUG: %*s%s (%s%s) %s\n", indent, "", attr->name,
            attr->num_values > 1 ? "1setOf " : "",
	    ippTagString(attr->value_tag), buffer);
  }
}