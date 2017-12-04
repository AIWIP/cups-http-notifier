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
#include <json-c/json.h>
#include <signal.h>
#include <stdio.h>
 
 /*
  * Local functions...
  */
 
  /*
   * 'generate_request_body()' - Generate request body for webhook...
   */

  char *generate_request_body(ipp_t *ipp, //* I - IPP request *
                              char *body) //* B - Body of webhook request *
  {
    cups_lang_t	    *lang;			                              /* Language info */
    ipp_tag_t		    group;		                                /* Current group */
    ipp_attribute_t	*attr;		                                /* Current attribute */
    char			      buffer[1024];	                            /* Value buffer */
    json_object     *request_json = json_object_new_object(); /* Root JSON object */
    json_object     *group_json = request_json;               /* Current JSON object for attributes */
    json_object     *json_value;                              /* Current JSON object for response */

    if ((lang = cupsLangDefault()) == NULL)
      exit(1);

    json_object *subject_value = json_object_new_string(cupsNotifySubject(lang, ipp));
    json_object_object_add(request_json, "subject", subject_value);

    json_object *message_value = json_object_new_string(cupsNotifyText(lang, ipp));
    json_object_object_add(request_json, "message", message_value);

    for (group = IPP_TAG_ZERO, attr = ipp->attrs; attr; attr = attr->next)
    {
      if (attr->group_tag == IPP_TAG_ZERO || !attr->name)
      {
        group = IPP_TAG_ZERO;
        continue;
      }

      if (group != attr->group_tag)
      {
        group = attr->group_tag;

        group_json = json_object_new_object();
        json_object_object_add(request_json, ippTagString(group), group_json);
      }

      ippAttributeString(attr, buffer, sizeof(buffer));


      // IPP_TAG_CUPS_INVALID = -1,		/* Invalid tag name for @link ippTagValue@ */
      // IPP_TAG_OPERATION,			/* Operation group */
      // IPP_TAG_JOB,				/* Job group */
      // IPP_TAG_END,				/* End-of-attributes */
      // IPP_TAG_PRINTER,			/* Printer group */
      // IPP_TAG_UNSUPPORTED_GROUP,		/* Unsupported attributes group */
      // IPP_TAG_SUBSCRIPTION,			/* Subscription group */
      // IPP_TAG_EVENT_NOTIFICATION,		/* Event group */
      // IPP_TAG_RESOURCE,			/* Resource group @private@ */
      // IPP_TAG_DOCUMENT,			/* Document group */
      // IPP_TAG_UNSUPPORTED_VALUE = 0x10,	/* Unsupported value */
      // IPP_TAG_DEFAULT,			/* Default value */
      // IPP_TAG_UNKNOWN,			/* Unknown value */
      // IPP_TAG_NOVALUE,			/* No-value value */
      // IPP_TAG_NOTSETTABLE = 0x15,		/* Not-settable value */
      // IPP_TAG_DELETEATTR,			/* Delete-attribute value */
      // IPP_TAG_ADMINDEFINE,			/* Admin-defined value */
      // IPP_TAG_BOOLEAN,			/* Boolean value */
      // IPP_TAG_ENUM,				/* Enumeration value */
      // IPP_TAG_STRING = 0x30,		/* Octet string value */
      // IPP_TAG_DATE,				/* Date/time value */
      // IPP_TAG_RESOLUTION,			/* Resolution value */
      // IPP_TAG_RANGE,			/* Range value */
      // IPP_TAG_BEGIN_COLLECTION,		/* Beginning of collection value */
      // IPP_TAG_TEXTLANG,			/* Text-with-language value */
      // IPP_TAG_NAMELANG,			/* Name-with-language value */
      // IPP_TAG_END_COLLECTION,		/* End of collection value */
      // IPP_TAG_TEXT = 0x41,			/* Text value */
      // IPP_TAG_NAME,				/* Name value */
      // IPP_TAG_RESERVED_STRING,		/* Reserved for future string value @private@ */
      // IPP_TAG_KEYWORD,			/* Keyword value */
      // IPP_TAG_URI,				/* URI value */
      // IPP_TAG_URISCHEME,			/* URI scheme value */
      // IPP_TAG_CHARSET,			/* Character set value */
      // IPP_TAG_LANGUAGE,			/* Language value */
      // IPP_TAG_MIMETYPE,			/* MIME media type value */
      // IPP_TAG_MEMBERNAME,			/* Collection member name value */
      // IPP_TAG_EXTENSION = 0x7f,		/* Extension point for 32-bit tags */

      // Integer
      if (attr->value_tag == IPP_TAG_INTEGER)
      {
        json_value = json_object_new_int (buffer);
      }

      // All Other Values
      else
        json_value = json_object_new_string(buffer);
      }
      
      json_object_object_add(group_json, (char *)attr->name, json_value);
    }

    strcpy(body, json_object_to_json_string(request_json));
  }
 
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
    CURL                *curl;              /* CURL HTTP Client */
    CURLcode            res;                /* HTTP Response */
    struct curl_slist   *headers=NULL;      /* HTTP Headers */
    char		            baseurl[1024];		  /* Base URL */
    char                request_body[40000];      /* Body for webhook request */

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

       
    httpAssembleURI(HTTP_URI_CODING_ALL, baseurl, sizeof(baseurl), scheme, NULL, host, port, resource);

    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
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

        generate_request_body(event, request_body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
            exit(0);

        /*
         * Free the memory used for this event...
         */

        if (request_body)
          free(request_body);

        if (curl)
          curl_easy_cleanup(curl);

        curl_slist_free_all(headers);

        ippDelete(event);
        event = NULL;
    }

    return (0);
 }
