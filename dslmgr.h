#ifndef DSLMGR_H
#define DSLMGR_H

/* ubus published objects */
extern struct ubus_object dsl_object;


extern int dslmgr_nl_msgs_handler(struct ubus_context *ctx);

#endif /* DSLMGR_H */
