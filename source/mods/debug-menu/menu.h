#include "../../runtime/native/input.h"
#ifndef MOON_DEBUG_MENU_H
#define MOON_DEBUG_MENU_H
#include "core.h"
static struct {
    PTR world_field,ui_field;
    PTR transform,position,up,alive,delta,frame,focused,restoring,blocked,menu_blocked,typing,set_blocked;
    PTR flying,set_flying,speed,set_speed,fly_speed,set_fly_speed,teleport;
    PTR joined,host,local,player_id,wallet,decimal_int,decimal_number,set_money;
    PTR component,build_begin,build_ensure,build_open,build_is_open,build_warming,holding_build_tool,shop_open,shop_is_open,find_objects,spawn,owner,driven,enter,exit,equipment_player;
    PTR cursor_lock,cursor_visible,set_lock,set_visible;
    short (*key)(int);PTR (*foreground)(void);DWORD (*window_pid)(PTR,DWORD *);
    DRoot world,walker;DMState state;DMPolicy policy;
    DWORD thread;int last_frame,owned_input,old_lock,old_visible,old_ui,can_build;
    int movement_changed,old_flying,fly;float old_speed,old_fly_speed,current_speed;
    DVec position_value;unsigned funds;int funds_ok;float fps;
    unsigned snapshots;int test_tools,last_action;const char *status;char mods[2048];
    DMAction pending;DMKey pending_world;int pending_frame,executing;
} dm;
static int dalive(PTR p){if(!p||!dm.alive)return 0;int ok=1;void *a[]={p};PTR v=call(dm.alive,0,a,&ok);return ok&&v&&*(unsigned char *)unbox(v);}
static PTR dcomponent(PTR walker,const char *type,int *ok){void *a[]={dtype("MoonCleaners",type)};return a[0]?call(dm.component,walker,a,ok):0;}
static DVec dpos(PTR component,int *ok){return dvector(dm.position,call(dm.transform,component,0,ok),ok);}
static unsigned dbalance(PTR world,int *ok){
    PTR id=call(dm.player_id,0,0,ok);void *a[]={id};DRoot value=dhold(id?call(dm.wallet,world,a,ok):0);
    if(!value){*ok=0;return 0;}void *args[]={unbox(dget(value))};PTR v=call(dm.decimal_number,0,args,ok);
    int amount=v?*(int *)unbox(v):0;if(!v)*ok=0;ddrop(&value);return amount>0?(unsigned)amount:0;
}
static int dhost(void){int ok=1;if(!boolean(dm.joined,0,&ok))return ok;PTR h=call(dm.host,0,0,&ok),l=call(dm.local,0,0,&ok);U64 a=0,b=0;if(h)dcopy(&a,unbox(h),8);if(l)dcopy(&b,unbox(l),8);return ok&&a&&a==b;}
static void dinput(int own){
    PTR walker=dget(dm.walker);int ok=1;
    if(own&&!dm.owned_input){
        unsigned char raw=0;field_value(walker,dm.ui_field,&raw);dm.old_ui=raw;
        dm.old_lock=integer(dm.cursor_lock,0,&ok);dm.old_visible=boolean(dm.cursor_visible,0,&ok);
        if(!ok){dm.state.open=0;return;}dm.owned_input=1;
    }
    if(own&&dm.owned_input){int none=0;unsigned char yes=1;void *a[]={&yes},*b[]={&none};call(dm.set_blocked,walker,a,&ok);call(dm.set_lock,0,b,&ok);call(dm.set_visible,0,a,&ok);}
    else if(dm.owned_input){
        unsigned char blocked=(unsigned char)dm.old_ui,visible=(unsigned char)dm.old_visible;void *a[]={&blocked},*b[]={&dm.old_lock},*c[]={&visible};
        if(dalive(walker))call(dm.set_blocked,walker,a,&ok);
        if(!walker||!boolean(dm.menu_blocked,walker,&ok)){call(dm.set_lock,0,b,&ok);call(dm.set_visible,0,c,&ok);}
        dm.owned_input=0;
    }
}
static void dgui_reset(void);
static void dclose(void){if(dm.state.open)event("menu-closed");dm.state.open=0;dm.state.queued=DM_NONE;dgui_reset();dinput(dm.pending!=DM_NONE);}
static void dmovement_restore(void){
    int ok=1;PTR walker=dget(dm.walker);if(dm.movement_changed&&dalive(walker)){
        unsigned char fly=(unsigned char)dm.old_flying,noclip=0;void *a[]={&fly,&noclip},*b[]={&dm.old_speed},*c[]={&dm.old_fly_speed};
        call(dm.set_flying,walker,a,&ok);call(dm.set_speed,walker,b,&ok);call(dm.set_fly_speed,walker,c,&ok);
    }dm.movement_changed=0;
}
static char *ddecimal(char *p,float f){if(f<0){*p++='-';f=-f;}if(!(f>=0&&f<10000000))return cat(p,"?");U64 n=(U64)(f*100);p=num(p,n/100);*p++='.';*p++=(char)('0'+n/10%10);*p++=(char)('0'+n%10);*p=0;return p;}
static void dsnapshot(const char *kind){
    static char b[1024];char *p=cat(b,"{\"event\":\"");p=cat(p,kind);p=cat(p,"\",\"thread\":");p=num(p,dm.thread);p=cat(p,",\"world\":");p=num(p,dm.state.world);p=cat(p,",\"host\":");p=num(p,(U64)dm.policy.host);
    p=cat(p,",\"action\":");p=num(p,(U64)dm.last_action);p=cat(p,",\"open\":");p=num(p,(U64)dm.state.open);
    p=cat(p,",\"action_name\":\"");p=cat(p,dm_action_name((DMAction)dm.last_action));p=cat(p,"\"");
    p=cat(p,",\"focused\":");p=num(p,(U64)dm.policy.focused);p=cat(p,",\"blocked\":");p=num(p,(U64)dm.policy.blocked);p=cat(p,",\"typing\":");p=num(p,(U64)dm.policy.typing);
    int catalog_ok=1;PTR builder=dcomponent(dget(dm.walker),"MoonDepotBuilder",&catalog_ok);
    p=cat(p,",\"catalog_open\":");p=num(p,(U64)(builder&&boolean(dm.build_is_open,builder,&catalog_ok)));
    p=cat(p,",\"catalog_warming\":");p=num(p,(U64)(builder&&boolean(dm.build_warming,builder,&catalog_ok)));
    int ok=1;unsigned char raw=0;field_value(dget(dm.walker),dm.ui_field,&raw);p=cat(p,",\"ui_blocked\":");p=num(p,raw);p=cat(p,",\"cursor_lock\":");p=num(p,(U64)integer(dm.cursor_lock,0,&ok));p=cat(p,",\"cursor_visible\":");p=num(p,(U64)boolean(dm.cursor_visible,0,&ok));
    p=cat(p,",\"funds\":");p=num(p,dm.funds);p=cat(p,",\"funds_ok\":");p=num(p,(U64)dm.funds_ok);p=cat(p,",\"flying\":");p=num(p,(U64)dm.fly);p=cat(p,",\"speed\":");p=ddecimal(p,dm.current_speed);
    p=cat(p,",\"position\":[");p=ddecimal(p,dm.position_value.x);p=cat(p,",");p=ddecimal(p,dm.position_value.y);p=cat(p,",");p=ddecimal(p,dm.position_value.z);p=cat(p,"]}\n");DWORD n=0;WriteFile(debug_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(debug_log);
}
static void dvehicle(PTR walker,int board,int *ok){
    int inactive=0,sort=0;const char *type=board?"MoonPrototypeVehicle":"MoonPrototypeEquipment";
    void *a[]={dtype("MoonCleaners",type),&inactive,&sort};DRoot objects=dhold(call(dm.find_objects,0,a,ok));
    PTR picked=0;float best=144;U64 count=objects?da.array_length(dget(objects)):0;
    if(count<=256)for(U64 i=0;i<count;i++){
        PTR item=*(PTR *)darray(dget(objects),i);if(call(board?dm.owner:dm.equipment_player,item,0,ok)!=walker)continue;
        if(!board){picked=item;break;}DVec v=dpos(item,ok),p=dm.position_value;float distance=(v.x-p.x)*(v.x-p.x)+(v.y-p.y)*(v.y-p.y)+(v.z-p.z)*(v.z-p.z);
        if(distance<best){best=distance;picked=item;}
    }
    PTR result=0;if(picked){int index=0;void *args[]={&index};result=board?call(boolean(dm.driven,picked,ok)?dm.exit:dm.enter,picked,0,ok):call(dm.spawn,picked,args,ok);}
    if(!result||!*(unsigned char *)unbox(result))*ok=0;ddrop(&objects);
}
static void daction(DMAction action){
    PTR walker=dget(dm.walker),world=dget(dm.world);int ok=1;
    if(action==DM_BUILD&&!dm.can_build){dm.status="Equip the building tool first";event("action-unavailable-3");return;}
    /* The release that activates our button must finish before a vanilla UI
     * sees input. Retain capture for three frames, then transfer ownership. */
    if(!dm.executing&&(action==DM_BUILD||action==DM_UPGRADES||action==DM_VEHICLE||action==DM_BOARD)){
        dm.pending=action;dm.pending_world=dm.state.world;dm.pending_frame=dm.last_frame+3;
        dm.status="Opening game controls";dclose();return;
    }
    if(action==DM_FUNDS_SMALL||action==DM_FUNDS_LARGE){
        int amount=action==DM_FUNDS_SMALL?10000:100000;void *a[]={&amount};DRoot value=dhold(call(dm.decimal_int,0,a,&ok));
        void *b[]={value?unbox(dget(value)):0};if(b[0])call(dm.set_money,world,b,&ok);else ok=0;ddrop(&value);
    }else if(action==DM_BUILD||action==DM_UPGRADES){
        dclose();PTR component=dcomponent(walker,action==DM_BUILD?"MoonDepotBuilder":"MoonUpgradeShop",&ok);unsigned char yes=1;void *a[]={&yes};
        if(!component)ok=0;else{if(action==DM_BUILD){call(dm.build_ensure,component,0,&ok);call(dm.build_begin,component,0,&ok);}call(action==DM_BUILD?dm.build_open:dm.shop_open,component,a,&ok);if(!boolean(action==DM_BUILD?dm.build_is_open:dm.shop_is_open,component,&ok))ok=0;}
    }else if(action==DM_FLIGHT||action==DM_SPEED_NORMAL||action==DM_SPEED_FAST||action==DM_SPEED_FASTER){
        if(!dm.movement_changed){dm.old_flying=boolean(dm.flying,walker,&ok);dm.old_speed=dnumber(dm.speed,walker,&ok);dm.old_fly_speed=dnumber(dm.fly_speed,walker,&ok);dm.movement_changed=ok;}
        if(action==DM_FLIGHT){unsigned char yes=(unsigned char)!boolean(dm.flying,walker,&ok),noclip=0;void *a[]={&yes,&noclip};call(dm.set_flying,walker,a,&ok);}
        else {float multiplier=action==DM_SPEED_NORMAL?1:action==DM_SPEED_FAST?3:8,fly=12*multiplier;void *a[]={&multiplier},*b[]={&fly};call(dm.set_speed,walker,a,&ok);call(dm.set_fly_speed,walker,b,&ok);}
    }else if(action==DM_RESET){dmovement_restore();}
    else if(action==DM_MARK){dm.state.mark[0]=dm.position_value.x;dm.state.mark[1]=dm.position_value.y;dm.state.mark[2]=dm.position_value.z;dm.state.has_mark=1;}
    else if(action==DM_RETURN||action==DM_LIFT){
        DVec position={dm.state.mark[0],dm.state.mark[1],dm.state.mark[2]};
        if(action==DM_LIFT){DVec up=dvector(dm.up,call(dm.transform,walker,0,&ok),&ok);position=(DVec){dm.position_value.x+3*up.x,dm.position_value.y+3*up.y,dm.position_value.z+3*up.z};}
        unsigned char refresh=1;void *a[]={&position,&refresh};call(dm.teleport,walker,a,&ok);
    }else if(action==DM_VEHICLE||action==DM_BOARD){dclose();dvehicle(walker,action==DM_BOARD,&ok);}
    else if(action==DM_SNAPSHOT){dm.snapshots++;dsnapshot("snapshot");}
    dm.status=ok?"Action complete":"Unavailable in the current situation";
    dm.last_action=(int)action;
    if(ok){dm.position_value=dpos(walker,&ok);dm.fly=boolean(dm.flying,walker,&ok);dm.current_speed=dnumber(dm.speed,walker,&ok);dm.funds_ok=1;dm.funds=dbalance(world,&dm.funds_ok);dsnapshot("action-state");}
    char text[80],*p=cat(text,ok?"action-ok-":"action-unavailable-");num(p,(U64)action);event(text);
}
static struct {
    PTR label,draw,get_color,set_color,skin,skin_label,style_copy,font_size,alignment,normal,text_color,white;
    PTR current,event_type,event_mouse,event_button,event_key,event_shift,use,width,height;
    DRoot style,texture;
    float scale,mx,my;int event,button,key,shift,focus,pressed,count,activation_held;
    struct {DRect r;const char *label;int enabled;int action;} controls[24];
} dg;
static void dgui_reset(void){dg.focus=0;dg.pressed=-1;dg.activation_held=0;}
static DRect dr(float x,float y,float w,float h){float s=dg.scale;return (DRect){16+x*s,16+y*s,w*s,h*s};}
static int dhit(DRect r){return dg.mx>=r.x&&dg.mx<r.x+r.w&&dg.my>=r.y&&dg.my<r.y+r.h;}
static void dcolor(DColor color,int *ok){void *a[]={&color};call(dg.set_color,0,a,ok);}
static void dfill(DRect r,DColor color,int *ok){dcolor(color,ok);void *a[]={&r,dget(dg.texture)};call(dg.draw,0,a,ok);}
static void dlabel(DRect r,const char *text,int *ok){dcolor((DColor){.92f,.92f,.92f,1},ok);void *a[]={&r,da.string(text),dget(dg.style)};call(dg.label,0,a,ok);}
static void dbutton(float x,float y,float w,const char *text,int action,int enabled,int *ok){
    int index=dg.count++;if(index>=24){*ok=0;return;}DRect r=dr(x,y,w,34);
    dg.controls[index].r=r;dg.controls[index].label=text;dg.controls[index].action=action;dg.controls[index].enabled=enabled;
    DColor color=enabled?(dhit(r)?(DColor){.27f,.28f,.29f,1}:(DColor){.19f,.20f,.21f,1}):(DColor){.12f,.13f,.14f,1};
    dfill(r,(DColor){.31f,.32f,.33f,1},ok);float border=dg.scale;r.x+=border;r.y+=border;r.w-=2*border;r.h-=2*border;dfill(r,color,ok);
    if(dg.focus==index&&enabled){DRect line=r;line.w=2*dg.scale;dfill(line,(DColor){.62f,.75f,.87f,1},ok);}
    r.x+=10*dg.scale;r.w-=20*dg.scale;r.y+=5*dg.scale;dlabel(r,text,ok);
}
static int dstyles(int *ok){
    if(dg.style)return 1;
    PTR skin=call(dg.skin,0,0,ok),source=skin?call(dg.skin_label,skin,0,ok):0;
    dg.style=dhold(da.new_object(find_class("UnityEngine","GUIStyle")));void *a[]={source};
    if(!source||!dg.style)return 0;call(dg.style_copy,dget(dg.style),a,ok);
    PTR state=call(dg.normal,dget(dg.style),0,ok);DColor text={.92f,.92f,.92f,1};void *b[]={&text};call(dg.text_color,state,b,ok);
    int align=0;void *c[]={&align};call(dg.alignment,dget(dg.style),c,ok);
    dg.texture=dhold(call(dg.white,0,0,ok));return *ok&&dg.texture;
}
static void dcontrol(int index){
    if(index<0||index>=dg.count||!dg.controls[index].enabled)return;
    int action=dg.controls[index].action;
    if(action<0){if(action==-1)dclose();else{dm.state.tab=-action-10;dg.focus=0;}}
    else if(!dm_queue(&dm.state,dm.policy,(DMAction)action))dm.status="Action is not available right now";
}
static void dgui(void){
    if(!dm.state.open||!dm.policy.ready)return;int ok=1;
    PTR current=call(dg.current,0,0,&ok);if(!current)return;
    dg.event=integer(dg.event_type,current,&ok);dg.button=integer(dg.event_button,current,&ok);dg.key=integer(dg.event_key,current,&ok);dg.shift=boolean(dg.event_shift,current,&ok);
    typedef struct {float x,y;} V2;V2 mouse={0};PTR pos=call(dg.event_mouse,current,0,&ok);if(pos)dcopy(&mouse,unbox(pos),sizeof(mouse));
    dg.mx=mouse.x;dg.my=mouse.y;
    int width=integer(dg.width,0,&ok),height=integer(dg.height,0,&ok);float sw=(float)(width-32)/448,sh=(float)(height-32)/620;
    dg.scale=sw<sh?sw:sh;if(dg.scale>1)dg.scale=1;if(dg.scale<.3f||!dstyles(&ok))return;
    DColor old_color={1,1,1,1};PTR old=call(dg.get_color,0,0,&ok);if(old)dcopy(&old_color,unbox(old),sizeof(old_color));
    int font=(int)(16*dg.scale);void *a[]={&font};call(dg.font_size,dget(dg.style),a,&ok);
    dg.count=0;dfill(dr(0,0,448,620),(DColor){.30f,.31f,.32f,1},&ok);dfill(dr(1,1,446,618),(DColor){.095f,.10f,.105f,.99f},&ok);
    dlabel(dr(18,14,300,30),"Debug Menu",&ok);dbutton(356,10,78,"Close",-1,1,&ok);
    dlabel(dr(18,46,410,26),dm.policy.host?"F4 toggle  /  Host controls":"F4 toggle  /  Guest inspection",&ok);
    dbutton(16,82,136,"Player",-10,1,&ok);dbutton(156,82,136,"World",-11,1,&ok);dbutton(296,82,136,"Inspect",-12,1,&ok);
    dfill(dr(16+140*(float)dm.state.tab,116,136,2),(DColor){.62f,.75f,.87f,1},&ok);
    dfill(dr(16,128,416,1),(DColor){.24f,.25f,.26f,1},&ok);
    int host=dm.policy.host&&!dm.policy.blocked&&!dm.policy.typing;
    if(dm.state.tab==0){
        dlabel(dr(18,144,410,25),"MOVEMENT",&ok);
        dbutton(16,178,416,dm.fly?"Fly mode: ON":"Fly mode: OFF",DM_FLIGHT,host,&ok);
        dlabel(dr(18,226,410,24),"Movement speed",&ok);
        dbutton(16,258,136,"1x",DM_SPEED_NORMAL,host,&ok);dbutton(156,258,136,"3x",DM_SPEED_FAST,host,&ok);dbutton(296,258,136,"8x",DM_SPEED_FASTER,host,&ok);
        dbutton(16,306,416,"Restore movement defaults",DM_RESET,host,&ok);
        dfill(dr(16,358,416,1),(DColor){.24f,.25f,.26f,1},&ok);dlabel(dr(18,374,410,24),"POSITION",&ok);
        dbutton(16,408,204,"Remember position",DM_MARK,1,&ok);dbutton(228,408,204,"Return to position",DM_RETURN,host&&dm.state.has_mark,&ok);
        dbutton(16,456,416,"Move up 3 m",DM_LIFT,host,&ok);
    }else if(dm.state.tab==1){
        dlabel(dr(18,144,410,25),"SET TEAM FUNDS",&ok);dbutton(16,178,204,"10,000",DM_FUNDS_SMALL,host,&ok);dbutton(228,178,204,"100,000",DM_FUNDS_LARGE,host,&ok);
        dfill(dr(16,230,416,1),(DColor){.24f,.25f,.26f,1},&ok);dlabel(dr(18,246,410,25),"CATALOGS",&ok);
        dbutton(16,280,204,dm.can_build?"Build catalog":"Equip building tool",DM_BUILD,host&&dm.can_build,&ok);dbutton(228,280,204,"Upgrade shop",DM_UPGRADES,host,&ok);
        dfill(dr(16,332,416,1),(DColor){.24f,.25f,.26f,1},&ok);dlabel(dr(18,348,410,25),"VEHICLES",&ok);
        dbutton(16,382,416,"Spawn starter vehicle",DM_VEHICLE,host,&ok);dbutton(16,430,416,"Enter / exit nearby owned vehicle",DM_BOARD,host,&ok);
        dlabel(dr(18,482,410,24),"Catalog and vehicle actions close this panel.",&ok);
    }else{
        char line[128],*p=cat(line,"FPS  ");p=num(p,(U64)(dm.fps>0?dm.fps:0));p=cat(p,"     Speed  ");ddecimal(p,dm.current_speed);dlabel(dr(18,144,410,26),line,&ok);
        p=cat(line,"Position  ");p=ddecimal(p,dm.position_value.x);p=cat(p,"  ");p=ddecimal(p,dm.position_value.y);p=cat(p,"  ");ddecimal(p,dm.position_value.z);dlabel(dr(18,180,410,26),line,&ok);
        p=cat(line,"Funds  ");if(dm.funds_ok)num(p,dm.funds);else cat(p,"unavailable");dlabel(dr(18,216,410,26),line,&ok);
        dbutton(16,260,416,"Write diagnostic snapshot",DM_SNAPSHOT,1,&ok);
        dlabel(dr(18,310,410,26),"Active mods",&ok);dlabel(dr(18,346,410,162),dm.mods,&ok);
    }
    dfill(dr(16,538,416,1),(DColor){.24f,.25f,.26f,1},&ok);dlabel(dr(18,552,410,26),dm.status?dm.status:"Ready",&ok);dlabel(dr(18,584,410,26),"Tab: focus   Enter: activate   F4: close",&ok);
    if(dg.focus>=dg.count)dg.focus=0;
    if(dg.event==0&&dg.button==0){dg.pressed=-1;for(int i=0;i<dg.count;i++)if(dg.controls[i].enabled&&dhit(dg.controls[i].r)){dg.focus=dg.pressed=i;break;}}
    if(dg.event==1&&dg.button==0){int pressed=dg.pressed;dg.pressed=-1;if(pressed>=0&&pressed<dg.count&&dhit(dg.controls[pressed].r))dcontrol(pressed);}
    if(dg.event==4){
        if(dg.key==9){for(int n=0;n<dg.count;n++){dg.focus=(dg.focus+dg.count+(dg.shift?-1:1))%dg.count;if(dg.controls[dg.focus].enabled)break;}}
        if(dg.key==13||dg.key==32||dg.key==271){if(!dg.activation_held)dcontrol(dg.focus);dg.activation_held=1;}
        call(dg.use,current,0,&ok);
    }else if(dg.event==5&&(dg.key==13||dg.key==32||dg.key==271))dg.activation_held=0;
    else if((dg.event==0||dg.event==1||dg.event==3||dg.event==6)&&dhit(dr(0,0,448,620)))call(dg.use,current,0,&ok);
    dcolor(old_color,&ok);if(!ok){dclose();event("gui-call-failed");}
}
static int debug_before(int id,Context *context){
    if(id==1){if(dm.thread==GetCurrentThreadId())dgui();return 0;}
    if(!dm.thread){dm.thread=GetCurrentThreadId();event("main-thread");}if(dm.thread!=GetCurrentThreadId())return 0;
    int ok=1,frame=integer(dm.frame,0,&ok);if(!ok||frame==dm.last_frame)return 0;dm.last_frame=frame;
    PTR walker=(PTR)context->rcx,world=0;field_value(walker,dm.world_field,&world);
    if(world!=dget(dm.world)||walker!=dget(dm.walker)){
        dm.pending=DM_NONE;dclose();dmovement_restore();ddrop(&dm.world);ddrop(&dm.walker);dm_world(&dm.state,(U64)world);
        dm.world=dhold(world);dm.walker=dhold(walker);dm.status="Ready";if(world)event("world-session");
    }
    PTR window=dm.foreground();DWORD pid=0;if(window)dm.window_pid(window,&pid);
    /* MenuBlocked includes uiBlocked. Exclude only our own captured flag while
     * sampling external menus, then reacquire it before vanilla Update runs. */
    if(dm.owned_input){unsigned char raw=(unsigned char)dm.old_ui;void *a[]={&raw};call(dm.set_blocked,walker,a,&ok);}
    dm.policy=(DMPolicy){world&&dalive(world)&&!boolean(dm.restoring,world,&ok),dhost(),pid==GetCurrentProcessId()&&boolean(dm.focused,0,&ok),boolean(dm.typing,0,&ok),boolean(dm.menu_blocked,walker,&ok)};
    if(!ok)dm.policy.ready=0;
    if(dm.pending&&(!dm.policy.ready||!dm.policy.focused||!dm.policy.host||dm.pending_world!=dm.state.world))dm.pending=DM_NONE;
    int open=dm.state.open;dm_toggle(&dm.state,dm.policy,moon_input_action("debug-menu.toggle","F4",1).held);
    if(open!=dm.state.open){dgui_reset();event(dm.state.open?"menu-open":"menu-closed");}dinput(dm.state.open||dm.pending!=DM_NONE);
    if(dm.policy.ready){
        dm.position_value=dpos(walker,&ok);dm.fly=boolean(dm.flying,walker,&ok);dm.current_speed=dnumber(dm.speed,walker,&ok);
        PTR builder=dcomponent(walker,"MoonDepotBuilder",&ok);dm.can_build=builder&&boolean(dm.holding_build_tool,builder,&ok);
        float delta=dnumber(dm.delta,0,&ok);if(delta>0)dm.fps=.9f*dm.fps+.1f/delta;
        if(dm.state.open){dm.funds_ok=1;dm.funds=dbalance(world,&dm.funds_ok);DMAction a=dm_take(&dm.state,dm.policy);if(a)daction(a);}
        if(dm.pending&&frame>=dm.pending_frame){DMAction a=dm.pending;dm.pending=DM_NONE;dinput(0);if(!dm.policy.blocked&&!dm.policy.typing){dm.executing=1;daction(a);dm.executing=0;}else dm.status="A game menu blocked the action";}
        if(dm.test_tools&&frame%60==0)dsnapshot("sample");
    }
    return 0;
}
static int debug_start(PTR game){
#define DB(F,NS,T,N,S) dm.F=dmethod(NS,T,N,S);if(!dm.F){event(T "::" N);return 0;}
    DB(transform,"UnityEngine","Component","get_transform","");DB(position,"UnityEngine","Transform","get_position","");DB(up,"UnityEngine","Transform","get_up","");DB(alive,"UnityEngine","Object","op_Implicit","UnityEngine.Object");
    DB(frame,"UnityEngine","Time","get_frameCount","");DB(delta,"UnityEngine","Time","get_unscaledDeltaTime","");DB(focused,"UnityEngine","Application","get_isFocused","");
    DB(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");DB(blocked,"MoonCleaners","MoonWalker","get_UIBlocked","");DB(menu_blocked,"MoonCleaners","MoonWalker","get_MenuBlocked","");DB(typing,"MoonCleaners","MoonInput","get_Typing","");DB(set_blocked,"MoonCleaners","MoonWalker","set_UIBlocked","System.Boolean");
    DB(flying,"MoonCleaners","MoonWalker","get_Flying","");DB(set_flying,"MoonCleaners","MoonWalker","SetFlying","System.Boolean;System.Boolean");DB(speed,"MoonCleaners","MoonWalker","get_DevSpeedMultiplier","");DB(set_speed,"MoonCleaners","MoonWalker","set_DevSpeedMultiplier","System.Single");DB(fly_speed,"MoonCleaners","MoonWalker","get_FlySpeed","");DB(set_fly_speed,"MoonCleaners","MoonWalker","set_FlySpeed","System.Single");DB(teleport,"MoonCleaners","MoonWalker","Teleport","UnityEngine.Vector3;System.Boolean");
    DB(joined,"MoonCleaners","MoonCrew","get_Joined","");DB(host,"MoonCleaners","MoonCrew","get_HostId","");DB(local,"Steamworks","SteamUser","GetSteamID","");DB(player_id,"MoonCleaners","MoonCrew","get_LocalPlayerId","");DB(wallet,"MoonCleaners","MoonWorld","WalletDollars","System.String");DB(set_money,"MoonCleaners","MoonWorld","SetTeamDollars","System.Decimal");DB(decimal_int,"System","Decimal","op_Implicit","System.Int32");DB(decimal_number,"System","Decimal","ToInt32","System.Decimal");
    DB(component,"UnityEngine","Component","GetComponent","System.Type");DB(build_begin,"MoonCleaners","MoonDepotBuilder","BeginPlacement","");DB(build_ensure,"MoonCleaners","MoonDepotBuilder","EnsureBuildUI","");DB(build_open,"MoonCleaners","MoonDepotBuilder","SetOpen","System.Boolean");DB(build_is_open,"MoonCleaners","MoonDepotBuilder","get_IsOpen","");DB(build_warming,"MoonCleaners","MoonDepotBuilder","get_WarmingMenu","");DB(shop_open,"MoonCleaners","MoonUpgradeShop","SetOpen","System.Boolean");DB(shop_is_open,"MoonCleaners","MoonUpgradeShop","get_IsOpen","");
    DB(holding_build_tool,"MoonCleaners","MoonDepotBuilder","get_HoldingBuildTool","");
    DB(find_objects,"UnityEngine","Object","FindObjectsByType","System.Type;UnityEngine.FindObjectsInactive;UnityEngine.FindObjectsSortMode");DB(spawn,"MoonCleaners","MoonPrototypeEquipment","SpawnVehicle","System.Int32");DB(equipment_player,"MoonCleaners","MoonPrototypeEquipment","get_Player","");DB(owner,"MoonCleaners","MoonPrototypeVehicle","get_Owner","");DB(driven,"MoonCleaners","MoonPrototypeVehicle","get_IsDriven","");DB(enter,"MoonCleaners","MoonPrototypeVehicle","Enter","");DB(exit,"MoonCleaners","MoonPrototypeVehicle","Exit","");
    DB(cursor_lock,"UnityEngine","Cursor","get_lockState","");DB(cursor_visible,"UnityEngine","Cursor","get_visible","");DB(set_lock,"UnityEngine","Cursor","set_lockState","UnityEngine.CursorLockMode");DB(set_visible,"UnityEngine","Cursor","set_visible","System.Boolean");
#undef DB
#define DG(F,T,N,S) dg.F=dmethod("UnityEngine",T,N,S);if(!dg.F){event(T "::" N);return 0;}
    DG(label,"GUI","Label","UnityEngine.Rect;System.String;UnityEngine.GUIStyle");DG(draw,"GUI","DrawTexture","UnityEngine.Rect;UnityEngine.Texture");DG(get_color,"GUI","get_color","");DG(set_color,"GUI","set_color","UnityEngine.Color");DG(skin,"GUI","get_skin","");DG(skin_label,"GUISkin","get_label","");DG(style_copy,"GUIStyle",".ctor","UnityEngine.GUIStyle");DG(font_size,"GUIStyle","set_fontSize","System.Int32");DG(alignment,"GUIStyle","set_alignment","UnityEngine.TextAnchor");DG(normal,"GUIStyle","get_normal","");DG(text_color,"GUIStyleState","set_textColor","UnityEngine.Color");DG(white,"Texture2D","get_whiteTexture","");
    DG(current,"Event","get_current","");DG(event_type,"Event","get_type","");DG(event_mouse,"Event","get_mousePosition","");DG(event_button,"Event","get_button","");DG(event_key,"Event","get_keyCode","");DG(event_shift,"Event","get_shift","");DG(use,"Event","Use","");DG(width,"Screen","get_width","");DG(height,"Screen","get_height","");
#undef DG
    dm.world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");dm.ui_field=get_field(find_class("MoonCleaners","MoonWalker"),"uiBlocked");if(!dm.world_field||!dm.ui_field)return 0;
    const unsigned char *p=(PTR)da.array_length;int displacement=0;if(p[0]!=0xe9)return 0;dcopy(&displacement,p+1,4);p+=5+displacement;if(p[0]!=0x8b||p[1]!=0x41||p[2]!=0x18||p[3]!=0xc3)return 0;
    PTR user=LoadLibraryA("user32.dll");dm.key=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");dm.foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");dm.window_pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");if(!dm.key||!dm.foreground||!dm.window_pid)return 0;
    DWORD n=GetEnvironmentVariableA("MOONLAB_MODS",dm.mods,sizeof(dm.mods));if(!n||n>=sizeof(dm.mods))cat(dm.mods,"debug-menu");else for(char *s=dm.mods;*s;s++)if(*s==';')*s='\n';
    dm.last_frame=-1;dm.status="Ready";dg.pressed=-1;
    char test[8];dm.test_tools=GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",test,sizeof(test))==1&&test[0]=='1';
    if(GetEnvironmentVariableA("MOONLAB_DEBUG_TELEMETRY",test,sizeof(test))==1&&test[0]=='1')dm.test_tools=1;
    if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game)||!prepare_hook(method("MoonCleaners","MoonDepotBuilder","OnGUI",0),100663833,1,game)){event("hook-shape-failed");return 0;}
    before_hook=debug_before;if(!install_hooks()){event("hook-install-failed");return 0;}event("ready");return 1;
}
#endif
