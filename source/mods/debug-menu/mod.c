#include "../../runtime/native/gameplay.h"
#include "api.h"
int _fltused=0;
static PTR debug_log;
static void event(const char *s){char b[512],*p=cat(b,"{\"event\":\"");p=cat(p,s);p=cat(p,"\"}\n");DWORD n=0;WriteFile(debug_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(debug_log);}
static void inventory(void){
    const char *types[]={"GUI","GUIStyle","GUISkin","GUIStyleState","GUIUtility","Event","Cursor","Screen","Texture2D","Matrix4x4"};
    for(unsigned t=0;t<sizeof(types)/sizeof(*types);t++){
        PTR k=find_class("UnityEngine",types[t]),it=0,m;
        while(k&&(m=da.methods(k,&it))){
            static char b[2048];char *p=cat(b,"{\"type\":\"");p=cat(p,types[t]);p=cat(p,"\",\"method\":\"");p=cat(p,da.name(m));p=cat(p,"\",\"parameters\":[");
            DWORD count=da.arity(m);for(DWORD i=0;i<count;i++){if(i)p=cat(p,",");p=cat(p,"\"");char *s=da.type_name(da.param(m,i));if(s){p=cat(p,s);da.free(s);}p=cat(p,"\"");}
            p=cat(p,"]}\n");DWORD n=0;WriteFile(debug_log,b,(DWORD)(p-b),&n,0);
        }
    }
    const char *owners[]={"MoonDepotBuilder","MoonInput","MoonInput","MoonWalker"};
    const char *names[]={"OnGUI","get_PlayKeyboard","get_PlayMouse","SetFlying"};
    const char *(*parameter_name)(PTR,DWORD)=(const char *(*)(PTR,DWORD))GetProcAddress(GetModuleHandleA("GameAssembly.dll"),"il2cpp_method_get_param_name");
    for(unsigned i=0;i<4;i++){
        PTR m=method("MoonCleaners",owners[i],names[i],i==3?2:0);if(!m)continue;
        unsigned char *code=*(unsigned char **)m;char b[512],*p=cat(b,"{\"owner\":\"");p=cat(p,owners[i]);p=cat(p,"\",\"method\":\"");p=cat(p,names[i]);p=cat(p,"\",\"token\":");p=num(p,method_token(m));p=cat(p,",\"bytes\":[");
        for(int j=0;j<5;j++){if(j)p=cat(p,",");p=num(p,code[j]);}p=cat(p,"],\"parameter_names\":[");
        for(DWORD j=0;j<da.arity(m);j++){if(j)p=cat(p,",");p=cat(p,"\"");if(parameter_name)p=cat(p,parameter_name(m,j));p=cat(p,"\"");}p=cat(p,"]}\n");DWORD n=0;WriteFile(debug_log,b,(DWORD)(p-b),&n,0);
    }
}
#include "menu.h"
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    static char folder[1000],path[1100];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,sizeof(folder));if(!n||n>=sizeof(folder))return 0;
    cat(cat(path,folder),"\\debug-menu.jsonl");debug_log=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(debug_log==(PTR)(U64)-1)return 0;
    if(!bind_runtime(domain,game)||!dapi(game)){event(da.failed?da.failed:"runtime-bind-failed");return 0;}
    char test[8];if(GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",test,sizeof(test))==1&&test[0]=='1')inventory();
    (void)dzero;return debug_start(game);
}
