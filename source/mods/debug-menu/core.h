#ifndef MOON_DEBUG_CORE_H
#define MOON_DEBUG_CORE_H
typedef unsigned long long DMKey;
typedef enum {
    DM_NONE,DM_FUNDS_SMALL,DM_FUNDS_LARGE,DM_BUILD,DM_UPGRADES,
    DM_FLIGHT,DM_SPEED_NORMAL,DM_SPEED_FAST,DM_SPEED_FASTER,DM_RESET,
    DM_MARK,DM_RETURN,DM_LIFT,DM_VEHICLE,DM_BOARD,DM_SNAPSHOT
} DMAction;
typedef struct {int ready,host,focused,typing,blocked;} DMPolicy;
typedef struct {
    DMKey world,queued_world;DMAction queued;
    int open,was_toggle,tab,has_mark;
    float mark[3];unsigned commands,rejected;
} DMState;
static inline const char *dm_action_name(DMAction a){
    static const char *const names[]={"none","funds-10000","funds-100000","build-catalog","upgrade-shop","fly-toggle","speed-1","speed-3","speed-8","movement-reset","remember-position","return-position","move-up","spawn-vehicle","board-vehicle","snapshot"};
    return a>=DM_NONE&&a<=DM_SNAPSHOT?names[a]:"unknown";
}
static inline int dm_mutates(DMAction a){return a!=DM_MARK&&a!=DM_SNAPSHOT;}
static inline int dm_allowed(const DMState *s,DMPolicy p,DMAction a){
    return a>DM_NONE&&a<=DM_SNAPSHOT&&s->world&&p.ready&&p.focused&&!p.typing&&!p.blocked
        &&(!dm_mutates(a)||p.host)&&(a!=DM_RETURN||s->has_mark);
}
static inline void dm_world(DMState *s,DMKey world){
    if(s->world==world)return;
    s->world=world;s->queued=DM_NONE;s->queued_world=0;s->open=0;s->has_mark=0;
}
static inline void dm_toggle(DMState *s,DMPolicy p,int down){
    if(!p.focused||!p.ready)s->open=0;
    else if(down&&!s->was_toggle&&(s->open||(!p.typing&&!p.blocked)))s->open=!s->open;
    if(!s->open)s->queued=DM_NONE;
    s->was_toggle=down;
}
static inline int dm_queue(DMState *s,DMPolicy p,DMAction action){
    if(!s->open||s->queued||!dm_allowed(s,p,action)){s->rejected++;return 0;}
    s->queued=action;s->queued_world=s->world;return 1;
}
static inline DMAction dm_take(DMState *s,DMPolicy p){
    DMAction a=s->queued;s->queued=DM_NONE;
    if(!a)return DM_NONE;
    if(!s->open||s->queued_world!=s->world||!dm_allowed(s,p,a)){s->rejected++;return DM_NONE;}
    s->commands++;return a;
}
#endif
