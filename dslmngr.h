#ifndef DSLMNGR_H
#define DSLMNGR_H

/* ubus published objects */
extern struct ubus_object dsl_object;


extern int dslmngr_nl_msgs_handler(struct ubus_context *ctx);

#endif /* DSLMNGR_H */
