/* Explicit, read-only scene census. Never run from the telemetry frame sampler. */
#ifndef MOON_DEBUG_RENDER_SNAPSHOT_H
#define MOON_DEBUG_RENDER_SNAPSHOT_H
#define DR_SCAN_MAX 8192u
#define DR_RENDER_MAX 512u
#define DR_ANCESTRY_MAX 12u
static struct {
    PTR id,name,object,parent,active,rotation,scale,enabled,renderer_enabled,near_clip,far_clip,depth,mask,shared_material,shared_mesh,bindings_valid,light_type,shadows,intensity,range,cast_mode,force_off,layer,render_layers,bounds,shader,queue,vertex_count,submesh_count,index_count,readable,vertices,triangles;
    PTR (*chars)(PTR);int (*length)(PTR);
    int bound,busy,write_failed;const char *missing;
} dr_native;
static char *dr_int(char *p,int value){if(value<0){*p++='-';return num(p,(U64)(-(long long)value));}return num(p,(U64)value);}
static char *dr_float(char *p,float value){
    if(!(value>=-10000000&&value<=10000000))return cat(p,"null");
    if(value<0){*p++='-';value=-value;}U64 n=(U64)((double)value*1000000+.5);p=num(p,n/1000000);*p++='.';
    for(U64 place=100000;place;place/=10){*p++=(char)('0'+(n/place)%10);}
    *p=0;return p;
}
/* JSON keeps names bounded even when a scene object's name is user supplied. */
static char *dr_name(char *p,PTR object,int *ok){
    PTR s=call(dr_native.name,object,0,ok);*p++='"';
    int count=s?dr_native.length(s):0;const unsigned short *chars=s?(const unsigned short *)dr_native.chars(s):0;
    if(count>64)count=64;
    for(int i=0;chars&&i<count;i++){
        unsigned c=chars[i];if(c=='"'||c=='\\'){*p++='\\';*p++=(char)c;}
        else if(c>=32&&c<127)*p++=(char)c;else *p++='?';
    }*p++='"';*p=0;return p;
}
static char *dr_vec(char *p,DVec v){*p++='[';p=dr_float(p,v.x);*p++=',';p=dr_float(p,v.y);*p++=',';p=dr_float(p,v.z);*p++=']';*p=0;return p;}
static void dr_write(const char *line){DWORD size=0,n=0;while(line[size])size++;if(!WriteFile(debug_log,line,size,&n,0)||n!=size)dr_native.write_failed=1;}
static int dr_bind(void){
    if(dr_native.bound)return 1;
#define DR_BIND(F,T,N,S) dr_native.F=dmethod("UnityEngine",T,N,S);if(!dr_native.F){dr_native.missing=T "::" N;return 0;}
    DR_BIND(id,"Object","GetInstanceID","");DR_BIND(name,"Object","get_name","");
    DR_BIND(object,"Component","get_gameObject","");DR_BIND(parent,"Transform","get_parent","");
    DR_BIND(active,"GameObject","get_activeInHierarchy","");DR_BIND(rotation,"Transform","get_rotation","");DR_BIND(scale,"Transform","get_lossyScale","");
    DR_BIND(enabled,"Behaviour","get_enabled","");DR_BIND(renderer_enabled,"Renderer","get_enabled","");DR_BIND(near_clip,"Camera","get_nearClipPlane","");DR_BIND(far_clip,"Camera","get_farClipPlane","");
    DR_BIND(depth,"Camera","get_depth","");DR_BIND(mask,"Camera","get_cullingMask","");
    DR_BIND(light_type,"Light","get_type","");DR_BIND(shadows,"Light","get_shadows","");DR_BIND(intensity,"Light","get_intensity","");DR_BIND(range,"Light","get_range","");
    DR_BIND(cast_mode,"Renderer","get_shadowCastingMode","");DR_BIND(force_off,"Renderer","get_forceRenderingOff","");DR_BIND(render_layers,"Renderer","get_renderingLayerMask","");DR_BIND(bounds,"Renderer","get_bounds","");DR_BIND(layer,"GameObject","get_layer","");
    DR_BIND(shader,"Material","get_shader","");DR_BIND(queue,"Material","get_renderQueue","");DR_BIND(vertex_count,"Mesh","get_vertexCount","");DR_BIND(submesh_count,"Mesh","get_subMeshCount","");DR_BIND(index_count,"Mesh","GetIndexCount","System.Int32");DR_BIND(readable,"Mesh","get_isReadable","");DR_BIND(vertices,"Mesh","get_vertices","");DR_BIND(triangles,"Mesh","get_triangles","");
    DR_BIND(shared_material,"Renderer","get_sharedMaterial","");DR_BIND(shared_mesh,"MeshFilter","get_sharedMesh","");
#undef DR_BIND
    dr_native.bindings_valid=dmethod("MoonCleaners","MoonWorld","get_RenderingBindingsValid","");if(!dr_native.bindings_valid){dr_native.missing="MoonWorld::get_RenderingBindingsValid";return 0;}
    PTR game=GetModuleHandleA("GameAssembly.dll");dr_native.chars=(PTR(*)(PTR))GetProcAddress(game,"il2cpp_string_chars");dr_native.length=(int(*)(PTR))GetProcAddress(game,"il2cpp_string_length");
    if(!dr_native.chars||!dr_native.length){dr_native.missing="il2cpp_string_access";return 0;}dr_native.bound=1;return 1;
}
static int dr_id(PTR object,int *ok){
    if(!object)return 0;
    DRoot root=dhold(object);if(!root){*ok=0;return 0;}
    int id=integer(dr_native.id,object,ok);ddrop(&root);return id;
}
static char *dr_ancestry(char *p,PTR transform,int *ok){
    p=cat(p,",\"ancestry\":[");unsigned count=0;DRoot root=dhold(transform);if(transform&&!root)*ok=0;
    for(;transform&&count<DR_ANCESTRY_MAX;count++){
        if(count){*p++=',';}p=cat(p,"{\"id\":");p=dr_int(p,dr_id(transform,ok));p=cat(p,",\"name\":");p=dr_name(p,transform,ok);p=cat(p,"}");
        transform=call(dr_native.parent,transform,0,ok);DRoot next=dhold(transform);if(transform&&!next)*ok=0;ddrop(&root);root=next;if(!*ok)break;
    }ddrop(&root);p=cat(p,"],\"ancestry_limited\":");return num(p,transform!=0);
}
#include "render_geometry.h"
static void dr_object(const char *type,PTR item,unsigned snapshot,int renderer,int *ok){
    /* Twelve escaped 64-character names plus fixed fields fit within 4 KiB. */
    static char b[4096];char *p=cat(b,"{\"event\":\"render-object\",\"snapshot\":");p=num(p,snapshot);p=cat(p,",\"type\":\"");p=cat(p,type);p=cat(p,"\",\"id\":");p=dr_int(p,dr_id(item,ok));
    PTR transform=call(dm.transform,item,0,ok);DRoot tr_root=dhold(transform);PTR go=call(dr_native.object,item,0,ok);DRoot go_root=dhold(go);
    if(!tr_root||!go_root||!*ok){*ok=0;cat(p,",\"ok\":0}\n");dr_write(b);ddrop(&go_root);ddrop(&tr_root);return;}
    int active=go&&boolean(dr_native.active,go,ok);p=cat(p,",\"name\":");p=dr_name(p,item,ok);p=cat(p,",\"active\":");p=num(p,active);
    DVec position=dvector(dm.position,transform,ok),scale=dvector(dr_native.scale,transform,ok);p=cat(p,",\"position\":");p=dr_vec(p,position);p=cat(p,",\"scale\":");p=dr_vec(p,scale);
    float q[4]={0};PTR boxed=call(dr_native.rotation,transform,0,ok);if(boxed)dcopy(q,unbox(boxed),sizeof(q));else *ok=0;
    p=cat(p,",\"rotation\":[");for(unsigned j=0;j<4;j++){if(j)*p++=',';p=dr_float(p,q[j]);}p=cat(p,"]");
    if(deq(type,"MoonWorld")){
        p=cat(p,",\"restoring\":");p=num(p,boolean(dm.restoring,item,ok));p=cat(p,",\"rendering_bindings_valid\":");p=num(p,boolean(dr_native.bindings_valid,item,ok));
    }else if(deq(type,"Camera")){
        p=cat(p,",\"enabled\":");p=num(p,boolean(dr_native.enabled,item,ok));p=cat(p,",\"near\":");p=dr_float(p,dnumber(dr_native.near_clip,item,ok));p=cat(p,",\"far\":");p=dr_float(p,dnumber(dr_native.far_clip,item,ok));
        p=cat(p,",\"depth\":");p=dr_float(p,dnumber(dr_native.depth,item,ok));p=cat(p,",\"culling_mask\":");p=dr_int(p,integer(dr_native.mask,item,ok));
    }else if(deq(type,"Light")){
        p=cat(p,",\"enabled\":");p=num(p,boolean(dr_native.enabled,item,ok));p=cat(p,",\"light_type\":");p=dr_int(p,integer(dr_native.light_type,item,ok));
        p=cat(p,",\"shadows\":");p=dr_int(p,integer(dr_native.shadows,item,ok));p=cat(p,",\"intensity\":");p=dr_float(p,dnumber(dr_native.intensity,item,ok));p=cat(p,",\"range\":");p=dr_float(p,dnumber(dr_native.range,item,ok));
    }else if(renderer){
        int enabled=boolean(dr_native.renderer_enabled,item,ok),force_off=boolean(dr_native.force_off,item,ok),cast_mode=integer(dr_native.cast_mode,item,ok);
        p=cat(p,",\"enabled\":");p=num(p,enabled);p=cat(p,",\"force_off\":");p=num(p,force_off);
        p=cat(p,",\"cast_mode\":");p=dr_int(p,cast_mode);p=cat(p,",\"layer\":");p=dr_int(p,integer(dr_native.layer,go,ok));p=cat(p,",\"rendering_layers\":");p=num(p,(unsigned)integer(dr_native.render_layers,item,ok));
        DVec bounds[2]={{0,0,0},{0,0,0}};PTR box=call(dr_native.bounds,item,0,ok);if(box)dcopy(bounds,unbox(box),sizeof(bounds));else *ok=0;
        p=cat(p,",\"bounds_center\":");p=dr_vec(p,bounds[0]);p=cat(p,",\"bounds_extents\":");p=dr_vec(p,bounds[1]);
        PTR material=call(dr_native.shared_material,item,0,ok);DRoot material_root=dhold(material);if(material&&!material_root)*ok=0;
        p=cat(p,",\"material\":");p=dr_int(p,dr_id(material,ok));
        if(material){PTR shader=call(dr_native.shader,material,0,ok);DRoot shader_root=dhold(shader);if(shader&&!shader_root)*ok=0;
            p=cat(p,",\"shader\":");p=shader?dr_name(p,shader,ok):cat(p,"null");p=cat(p,",\"render_queue\":");p=dr_int(p,integer(dr_native.queue,material,ok));ddrop(&shader_root);}
        ddrop(&material_root);
        void *args[]={dtype("UnityEngine","MeshFilter")};PTR filter=call(dm.component,item,args,ok);DRoot filter_root=dhold(filter);if(filter&&!filter_root)*ok=0;
        PTR mesh=filter?call(dr_native.shared_mesh,filter,0,ok):0;DRoot mesh_root=dhold(mesh);if(mesh&&!mesh_root)*ok=0;
        p=cat(p,",\"mesh\":");p=dr_int(p,dr_id(mesh,ok));
        if(mesh){p=cat(p,",\"vertex_count\":");p=num(p,(unsigned)integer(dr_native.vertex_count,mesh,ok));p=cat(p,",\"submesh_count\":");p=num(p,(unsigned)integer(dr_native.submesh_count,mesh,ok));if(active&&enabled&&!force_off&&cast_mode!=3)dr_geometry_register(item,mesh,position,scale,q,ok);}
        ddrop(&mesh_root);ddrop(&filter_root);
    }
    p=dr_ancestry(p,transform,ok);p=cat(p,",\"ok\":");p=num(p,*ok!=0);cat(p,"}\n");dr_write(b);ddrop(&go_root);ddrop(&tr_root);
}
static void dr_census(void){
    if(dr_native.busy)return;
    if(!dr_bind()){event(dr_native.missing?dr_native.missing:"render-census-unavailable");return;}
    dr_native.busy=1;dr_native.write_failed=0;dr_geometry_clear();
    static const char *types[]={"MoonWorld","MoonCollector","MoonCollectorDoor","MoonWorkshopDoor","Camera","Renderer","Light"};
    static const unsigned caps[]={8,32,64,64,16,DR_RENDER_MAX,128};
    for(unsigned t=0;t<sizeof(types)/sizeof(*types);t++){
        int ok=1,inactive=1,sort=0,renderer=t==5;PTR type=dtype(t<4?"MoonCleaners":"UnityEngine",types[t]);
        void *args[]={type,&inactive,&sort};DRoot list=dhold(type?call(dm.find_objects,0,args,&ok):0);if(!list)ok=0;
        U64 total=list?da.array_length(dget(list)):0,scanned=0;unsigned emitted=0,limited=0;
        for(;ok&&scanned<total&&scanned<DR_SCAN_MAX;scanned++){
            PTR item=*(PTR *)darray(dget(list),scanned);if(!dalive(item))continue;
            if(renderer||t==6){PTR transform=call(dm.transform,item,0,&ok);DRoot root=dhold(transform);if(!root)ok=0;DVec point={0};if(ok)point=dvector(dm.position,transform,&ok);ddrop(&root);if(!ok)break;float x=point.x-dm.position_value.x,y=point.y-dm.position_value.y,z=point.z-dm.position_value.z;if(x*x+y*y+z*z>75.f*75.f)continue;}
            if(emitted>=caps[t]){limited=1;break;}
            dr_object(types[t],item,dm.snapshots,renderer,&ok);emitted++;
        }
        if(scanned<total)limited=1;
        char b[512],*p=cat(b,"{\"event\":\"render-census\",\"schema\":1,\"snapshot\":");p=num(p,dm.snapshots);p=cat(p,",\"type\":\"");p=cat(p,types[t]);p=cat(p,"\",\"total\":");p=num(p,total);p=cat(p,",\"scanned\":");p=num(p,scanned);p=cat(p,",\"emitted\":");p=num(p,emitted);p=cat(p,",\"limited\":");p=num(p,limited);p=cat(p,",\"ok\":");p=num(p,ok&&!dr_native.write_failed);cat(p,"}\n");dr_write(b);ddrop(&list);
    }dr_geometry_dump(dm.snapshots);dr_native.busy=0;FlushFileBuffers(debug_log);
}
#endif
