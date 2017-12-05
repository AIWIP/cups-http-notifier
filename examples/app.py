import cups
import json
import logging

from flask import Flask, request

logger = logging.getLogger(__name__)
flask_app = Flask(__name__)
cups_connection = cups.Connection()

def create_cups_subscription(connection):
    # Remove any other cups subscriptions and make
    # sure we only have one which points to this
    # webhook
    #
    current_subscription_id = connection.createSubscription(
        uri='/',
        recipient_uri='http://localhost:5000',
        events=['all']
    )

    for subscription in connection.getSubscriptions("/"):
        subscription_id = subscription["notify-subscription-id"]

        if subscription_id != current_subscription_id:
            connection.cancelSubscription(subscription_id)


@flask_app.route("/")
def index():
    return u"OK"

@flask_app.route('/', methods=['POST'])
def webhook():

    notification = json.loads(request.get_data())
    event = notification["event-notification-attributes-tag"]
    event_type = event["notify-subscribed-event"]

    print('Recieved {} event from CUPS'.format(event_type))
    print(event)

    return u"OK"

create_cups_subscription(cups_connection)