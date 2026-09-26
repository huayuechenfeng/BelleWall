#ifndef BELLEWALL_LIBRARY_DELETE_H
#define BELLEWALL_LIBRARY_DELETE_H
// Backend owns validation and exclusive session lock. Never leave a selection
// pointing at a deleted payload; partial deletion can be retried safely.
namespace BelleLibrary {
template<class Backend> void Delete(Backend& backend){
    backend.Check();
    if(backend.IsSelected())backend.ClearSelection();
    backend.RemoveMetadata();
    backend.RemovePayload();
}
}
#endif
