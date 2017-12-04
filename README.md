# cups-http-notifier

This is a low effort alternative to the `dbus` notifier that ships with CUPS.

This codebase implements a HTTP notifier for CUPs which allows you to consume IPP subscription events
via JSON over HTTP.

Just register a subscription with the CUPS API using the `http` protocol pointed to your webhook
endpoint on your webserver in order to recieve these events.

We currently only support JSON as the payload for the webhook event, if you require any other formats
please open an issue or submit a pull request.

This code is built for the Linux operating system only and currently lacks support for OS X.

# Examples

In `examples` you can find a implementation of a webhook written in python. This application
will simply register a subscription with cups for the HTTP notifier and log any events it
recieves.

You can trigger events by add or removing printers as well as submitting jobs.

# How to build

- Make sure you have `build-essential` and `cups` installed on your operating system
- `sudo ./configure`
- `make`

After following the steps above run `sudo make install` to install the notifier in CUPS.
