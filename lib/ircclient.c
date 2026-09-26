#include <dyad.h>
#include <token.c>

typedef struct IRCClient_ IRCClient;
typedef struct IRCMessage_ IRCMessage;
typedef struct IRCChannel_ IRCChannel;

#define IRCC_ON_MESSAGE(name) void name(IRCClient *C, IRCChannel *channel, IRCMessage *message)
typedef IRCC_ON_MESSAGE(IRCCOnMessageF);

enum
{
    IRCMessageType_Error = 0,
    // Timeouts.
    IRCMessageType_Status,
    // Normal channel messages.
    IRCMessageType_Message,
    // Connected and ready for JOIN.
    IRCMessageType_Registered,
    // Called when a new channel is added.
    IRCMessageType_ChannelAdded,
    // Called when a new channel is removed.
    IRCMessageType_ChannelRemoved,
};

typedef struct IRCMessage_
{
    int type;
    Token nick, user, host, command, channel_name, body;
}
IRCMessage;

#define IRCC_CHANNEL_NAME_MAX (256)

enum
{
	IRCChannelType_Status = 0,
	IRCChannelType_Group,
	IRCChannelType_Private
};

typedef struct IRCChannel_
{
	int type;
    IRCClient *client;
    char name[IRCC_CHANNEL_NAME_MAX];
    void *udata;
}
IRCChannel;

typedef struct IRCClient_
{
    dyad_Stream *stream;
    char nickname[256];
    char password[256];
    bool registered;
    IRCChannel channels[16];
    int channels_count;
    IRCCOnMessageF *on_message;
}
IRCClient;

void
ircc_message_parse_(IRCMessage *message, const char *data, int size)
{
    // :d7!d7samurai@253.185.249.62.customer.cdi.no PRIVMSG #random :test success
    // PING LAG1796749693

    Token input;
    input.begin = (char*)data;
    input.end = input.begin + size;

    int ok = 1;
    if (*input.begin == ':') {
        input.begin++;
        token_delimited_by_char(&input, &message->nick, '!');
        token_delimited_by_char(&input, &message->user, '@');
        token_delimited_by_char(&input, &message->host, ' ');
    }

    ok = ok && token_delimited_by_char(&input, &message->command, ' ');
    ok = ok && token_delimited_by_char(&input, &message->channel_name, ' ');
    // Skip whitespace
    token_delimited_by_char(&input, 0, ':');

    if (ok) {
        message->body = input;
    }
}

void
ircc_error(IRCClient *C, IRCChannel *channel, const char *body)
{
    IRCMessage message = {0};
    message.type = IRCMessageType_Error;
    token_set_string(&message.body, body, strlen(body));

    if (channel) {
        C->on_message(C, channel, &message);
    } else {
        channel = C->channels;
        C->on_message(C, channel, &message);
    }
}

IRCChannel *
ircc_channel_add(IRCClient *C, int type, const char *channel_name, int channel_name_length)
{
    if (C->channels_count == array_count(C->channels)) {
        ircc_error(C, 0, "Channels are maxed out.");
        return 0;
    }

	if (channel_name_length < 0) {
		channel_name_length = strlen(channel_name);
	}

	if (channel_name_length >= IRCC_CHANNEL_NAME_MAX) {
		ircc_error(C, 0, "Channel's name is too long.");
		return 0;
	}


    IRCChannel *channel = &C->channels[C->channels_count];
	C->channels_count++;
	memset(channel, 0, sizeof(IRCChannel));

	channel->client = C;

	memcpy(channel->name, channel_name, channel_name_length);
	channel->name[channel_name_length] = 0;

	IRCMessage message = { 0 };
	message.type = IRCMessageType_ChannelAdded;
	C->on_message(C, channel, &message);

    return channel;    
}

IRCChannel *
ircc_channel_find(IRCClient *C, const char *channel_name, int channel_name_length)
{
    if (channel_name == 0) {
        return &C->channels[0];
    }

    if (channel_name_length < 0) {
        channel_name_length = strlen(channel_name);
    }

    IRCChannel *channel = C->channels;
    for (int i = 0; i != C->channels_count; ++i, ++channel) {
        if (strncmp(channel->name, channel_name, channel_name_length) == 0) {
            return channel;
        }
    }
    return 0;
}

IRCChannel *
ircc_channel_join(IRCClient *C, const char *channel_name, int channel_name_length)
{
    ASSERT(channel_name && *channel_name);

    if (!C->registered) {
        ircc_error(C, 0, "Not registered yet.");
        return 0;
    }


    IRCChannel *channel = ircc_channel_add(C, IRCChannelType_Group, channel_name, channel_name_length);

    dyad_writef(C->stream, "JOIN %s\r\n", channel->name);

    return channel;
}

void
ircc_send_message(IRCChannel *channel, const char *body, int body_length)
{
    // PRIVMSG #channel :message
    if (channel && channel->name && *channel->name)
    {
		IRCClient *C = channel->client;
		IRCMessage message = {0};
        message.type = IRCMessageType_Message;
        token_set_string(&message.nick, C->nickname, -1);
        token_set_string(&message.body, body, body_length);
        token_set_string(&message.channel_name, channel->name, -1);

        // Loopback.
        C->on_message(C, channel, &message);

        if (C->stream) {
            dyad_writef(C->stream, "PRIVMSG %s :%b\r\n", channel->name, body, (int)body_length);
        } else {
            ircc_error(C, channel, "Unable to send message.");
        }
    }
}

static void
ircc_on_event_(dyad_Event *e)
{
	IRCMessage message = {0};
    IRCClient *C = (IRCClient*)e->udata;
    IRCChannel *channel;

    switch (e->type)
    {
    case DYAD_EVENT_CONNECT:
        printf("connected: %s\n", e->msg);

        if (*C->password) {
            dyad_writef(e->stream, "PASS %s\r\n", C->password);
        }

        dyad_writef(e->stream, "NICK %s\r\n", C->nickname);
        dyad_writef(e->stream, "USER %s %s bla :%s\r\n", C->nickname, C->nickname, C->nickname);
        break;
    case DYAD_EVENT_CLOSE:
        printf("close: %s\n", e->msg);
        break;
    case DYAD_EVENT_ERROR:
        printf("error: %s\n", e->msg);
        break;
    case DYAD_EVENT_TIMEOUT:
        printf("timeout: %s\n", e->msg);
        break;
    case DYAD_EVENT_LINE:
        printf("data: %s\n", e->data);
        if (!memcmp(e->data, "PING", 4)) {
            dyad_writef(e->stream, "PONG%s\r\n", e->data + 4);
        } else if (!C->registered && strstr(e->data, "001")) {
            C->registered = 1;
            message.type = IRCMessageType_Registered;
            C->on_message(C, &C->channels[0], &message);
        } else {
            message.type = IRCMessageType_Message;
			ircc_message_parse_(&message, e->data, e->size);
			channel = 0;
            if (message.channel_name.begin) {
				if (strncmp(message.channel_name.begin, C->nickname, token_length(&message.channel_name)) == 0) {
					// Private conversation.
					// Find channel by nickname.
					channel = ircc_channel_find(C, message.nick.begin, token_length(&message.nick));
					if (!channel) {
						channel = ircc_channel_add(C, IRCChannelType_Private, message.nick.begin, token_length(&message.nick));
					}
				} else {
					// Find channel by channel name.
					channel = ircc_channel_find(C, message.channel_name.begin, token_length(&message.channel_name));
				}
            }
            if (!channel) {
                channel = &C->channels[0];
            }
            C->on_message(C, channel, &message);
		}
        break;
    }
}

int
ircc_init(IRCClient *C, const char *nickname, const char *password, IRCCOnMessageF* on_message)
{
    memset(C, 0, sizeof(IRCClient));

    C->stream = dyad_newStream();
    C->on_message = on_message;

    if (nickname) {
        int length = strlen(nickname);
        memcpy(C->nickname, nickname, minimum(length, array_count(C->nickname)));
    }

    if (password) {
        int length = strlen(password);
        memcpy(C->password, password, minimum(length, array_count(C->password)));
    }

	// Add "status" channel.
	ircc_channel_add(C, IRCChannelType_Status, "*STATUS*", -1);

    dyad_addListener(C->stream, DYAD_EVENT_CONNECT, ircc_on_event_,   (void*)C);
    dyad_addListener(C->stream, DYAD_EVENT_CLOSE,   ircc_on_event_,   (void*)C);
    dyad_addListener(C->stream, DYAD_EVENT_ERROR,   ircc_on_event_,   (void*)C);
    dyad_addListener(C->stream, DYAD_EVENT_TIMEOUT, ircc_on_event_,   (void*)C);
    dyad_addListener(C->stream, DYAD_EVENT_LINE,    ircc_on_event_,   (void*)C);
    dyad_connect(C->stream, "irc.handmade.network", 7666);
    return 0;
}