#ifndef BELLEWALL_RENDERER_IDENTITY_H
#define BELLEWALL_RENDERER_IDENTITY_H
// Shared by C++, the resource compiler and package generation checks.
#define BW_RENDER_DLL_UID 0xe7b31136
#define BW_RENDER_IMPL_UID 0xe7b31137
#define BW_RENDER_CONFIG_UID 0x70031129
#define BW_RENDER_CONFIG_TEXT "0x70031129"
#define BW_RENDER_TYPE "bellewall-render-longrun"
#define BW_RENDER_SESSION_VERSION 2
#ifndef BELLEWALL_RESOURCE_BUILD
namespace BelleRenderer {
inline bool KnownVersion(unsigned version){return version==1||version==BW_RENDER_SESSION_VERSION;}
inline unsigned ConfigUid(unsigned version){return version==1?0x70031119u:version==BW_RENDER_SESSION_VERSION?BW_RENDER_CONFIG_UID:0;}
inline const char* ConfigText(unsigned version){return version==1?"0x70031119":version==BW_RENDER_SESSION_VERSION?BW_RENDER_CONFIG_TEXT:"";}
inline bool AcceptLongrun(unsigned version){return version==BW_RENDER_SESSION_VERSION;}
}
#endif
#endif
