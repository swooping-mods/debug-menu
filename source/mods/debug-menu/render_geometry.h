/* Geometry is emitted only for matching-name/pose renderer pairs. Native station
 * lighting deliberately creates split meshes, so names/vertices alone cannot
 * distinguish disjoint face subsets from duplicate visible triangles. */
#ifndef MOON_DEBUG_RENDER_GEOMETRY_H
#define MOON_DEBUG_RENDER_GEOMETRY_H
#define DR_DETAIL_MAX 64u
#define DR_VERTEX_MAX 32768u
#define DR_INDEX_MAX 196608u
typedef struct {DRoot mesh;int renderer,mesh_id,selected;DVec position,scale;float rotation[4];char name[65];} DRMeshRecord;
static DRMeshRecord dr_meshes[DR_RENDER_MAX];static unsigned dr_mesh_count,dr_selection_skipped;
static PTR dr_mesh_log;static int dr_mesh_write_failed;
static char dr_mesh_buffer[4096];static char *dr_mesh_cursor;
static void dr_geometry_clear(void){for(unsigned i=0;i<dr_mesh_count;i++)ddrop(&dr_meshes[i].mesh);dr_mesh_count=0;dr_selection_skipped=0;}
static float dr_abs(float f){return f<0?-f:f;}
static int dr_pose_valid(const DRMeshRecord *r){
    const float *position=&r->position.x,*scale=&r->scale.x;
    for(unsigned i=0;i<3;i++)if(!(position[i]>=-10000000&&position[i]<=10000000&&scale[i]>=-10000000&&scale[i]<=10000000))return 0;
    float norm=0;
    for(unsigned i=0;i<4;i++){if(!(r->rotation[i]>=-10000000&&r->rotation[i]<=10000000))return 0;norm+=r->rotation[i]*r->rotation[i];}
    return norm>0;
}
static int dr_pose_matches(const DRMeshRecord *a,const DRMeshRecord *b){
    if(!dr_pose_valid(a)||!dr_pose_valid(b))return 0;
    const float *pa=&a->position.x,*pb=&b->position.x,*sa=&a->scale.x,*sb=&b->scale.x;float same=0,opposite=0;
    for(unsigned i=0;i<3;i++)if(dr_abs(pa[i]-pb[i])>.0001f||dr_abs(sa[i]-sb[i])>.0001f)return 0;
    for(unsigned i=0;i<4;i++){float d=dr_abs(a->rotation[i]-b->rotation[i]),n=dr_abs(a->rotation[i]+b->rotation[i]);if(d>same)same=d;if(n>opposite)opposite=n;}
    return same<=.0001f||opposite<=.0001f;
}
static void dr_geometry_register(PTR renderer,PTR mesh,DVec position,DVec scale,const float *rotation,int *ok){
    if(!mesh||!*ok)return;
    if(dr_mesh_count>=DR_RENDER_MAX){dr_selection_skipped++;return;}
    DRMeshRecord *r=dr_meshes+dr_mesh_count;PTR s=call(dr_native.name,renderer,0,ok);int length=s?dr_native.length(s):0;
    if(!*ok||!s||length<0||length>64){dr_selection_skipped++;return;}
    const unsigned short *chars=s?(const unsigned short *)dr_native.chars(s):0;
    if(length&&!chars){dr_selection_skipped++;return;}
    /* Match only names represented exactly in the census. Lossy/truncated names
     * must leave geometry incomplete rather than produce a clean negative. */
    for(int i=0;i<length;i++){if(chars[i]<32||chars[i]>=127){dr_selection_skipped++;return;}r->name[i]=(char)chars[i];}r->name[length]=0;
    r->renderer=dr_id(renderer,ok);r->mesh_id=dr_id(mesh,ok);r->mesh=dhold(mesh);r->position=position;r->scale=scale;r->selected=0;dcopy(r->rotation,rotation,sizeof(r->rotation));
    if(!*ok||!r->mesh||!dr_pose_valid(r)){ddrop(&r->mesh);dr_selection_skipped++;*ok=0;return;}dr_mesh_count++;
    for(unsigned i=0;i+1<dr_mesh_count;i++)if(dr_meshes[i].renderer!=r->renderer&&deq(dr_meshes[i].name,r->name)&&dr_pose_matches(dr_meshes+i,r)){dr_meshes[i].selected=1;r->selected=1;}
}
static int dr_geometry_open(void){
    if(dr_mesh_log)return 1;
    static char path[1200];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",path,1000);if(!n||n>=1000)return 0;
    cat(path+n,"\\debug-render-meshes.jsonl");dr_mesh_log=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(dr_mesh_log==(PTR)(U64)-1){dr_mesh_log=0;return 0;}return 1;
}
static void dr_geometry_flush(void){
    DWORD n=0,size=(DWORD)(dr_mesh_cursor-dr_mesh_buffer);if(size&&(!WriteFile(dr_mesh_log,dr_mesh_buffer,size,&n,0)||n!=size))dr_mesh_write_failed=1;
    dr_mesh_cursor=dr_mesh_buffer;*dr_mesh_cursor=0;
}
static void dr_geometry_room(void){if(dr_mesh_cursor-dr_mesh_buffer>3800)dr_geometry_flush();}
static void dr_geometry_record(const DRMeshRecord *record,unsigned snapshot){
    int ok=1;PTR mesh=dget(record->mesh);unsigned vertices=(unsigned)integer(dr_native.vertex_count,mesh,&ok),indices=0;
    int readable=boolean(dr_native.readable,mesh,&ok),submeshes=integer(dr_native.submesh_count,mesh,&ok);DRoot positions=0,triangles=0;
    int limited=vertices>DR_VERTEX_MAX||submeshes<0||submeshes>64;
    for(int i=0;ok&&!limited&&i<submeshes;i++){void *args[]={&i};PTR value=call(dr_native.index_count,mesh,args,&ok);unsigned count=value?*(unsigned *)unbox(value):0;if(!value)ok=0;if(count>DR_INDEX_MAX-indices)limited=1;else indices+=count;}
    if(ok&&readable&&!limited){
        positions=dhold(call(dr_native.vertices,mesh,0,&ok));triangles=dhold(call(dr_native.triangles,mesh,0,&ok));
        if(!positions||!triangles||da.array_length(dget(positions))!=vertices||da.array_length(dget(triangles))!=indices||indices%3)ok=0;
    }
    dr_mesh_cursor=dr_mesh_buffer;char *p=cat(dr_mesh_cursor,"{\"event\":\"render-mesh\",\"schema\":1,\"snapshot\":");p=num(p,snapshot);p=cat(p,",\"renderer\":");p=dr_int(p,record->renderer);p=cat(p,",\"mesh\":");p=dr_int(p,record->mesh_id);
    p=cat(p,",\"vertex_count\":");p=num(p,vertices);p=cat(p,",\"index_count\":");p=num(p,indices);p=cat(p,",\"readable\":");p=num(p,readable);p=cat(p,",\"limited\":");p=num(p,limited);p=cat(p,",\"ok\":");p=num(p,ok!=0);dr_mesh_cursor=cat(p,",\"vertices\":[");
    if(ok&&readable&&!limited){
        const DVec *data=(const DVec *)((const unsigned char *)dget(positions)+32);
        for(unsigned i=0;i<vertices;i++){dr_geometry_room();if(i)*dr_mesh_cursor++=',';dr_mesh_cursor=dr_vec(dr_mesh_cursor,data[i]);}
    }dr_geometry_room();dr_mesh_cursor=cat(dr_mesh_cursor,"],\"triangles\":[");
    if(ok&&readable&&!limited){
        const unsigned *data=(const unsigned *)((const unsigned char *)dget(triangles)+32);
        for(unsigned i=0;i<indices;i++){dr_geometry_room();if(i)*dr_mesh_cursor++=',';dr_mesh_cursor=num(dr_mesh_cursor,data[i]);}
    }dr_mesh_cursor=cat(dr_mesh_cursor,"]}\n");dr_geometry_flush();ddrop(&triangles);ddrop(&positions);
}
static void dr_geometry_dump(unsigned snapshot){
    unsigned selected=0,emitted=0;for(unsigned i=0;i<dr_mesh_count;i++)selected+=dr_meshes[i].selected!=0;
    int opened=!selected||dr_geometry_open();dr_mesh_write_failed=0;
    if(opened&&selected)for(unsigned i=0;i<dr_mesh_count&&emitted<DR_DETAIL_MAX;i++)if(dr_meshes[i].selected){dr_geometry_record(dr_meshes+i,snapshot);emitted++;}
    /* Flush before the receipt: failed persistence cannot qualify geometry. */
    if(dr_mesh_log&&!FlushFileBuffers(dr_mesh_log))dr_mesh_write_failed=1;
    char b[512],*p=cat(b,"{\"event\":\"render-geometry\",\"schema\":1,\"snapshot\":");p=num(p,snapshot);p=cat(p,",\"selected\":");p=num(p,selected);p=cat(p,",\"emitted\":");p=num(p,emitted);p=cat(p,",\"selection_skipped\":");p=num(p,dr_selection_skipped);p=cat(p,",\"limited\":");p=num(p,emitted<selected);p=cat(p,",\"ok\":");p=num(p,opened&&!dr_mesh_write_failed&&!dr_native.write_failed&&!dr_selection_skipped);cat(p,"}\n");dr_write(b);
    dr_geometry_clear();
}
#endif
