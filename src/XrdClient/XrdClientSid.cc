//////////////////////////////////////////////////////////////////////////
//                                                                      //
// XrdClientMessage                                                     //
//                                                                      //
// Author: Fabrizio Furano (INFN Padova, 2005)                          //
//                                                                      //
// Utility classes to handle the mapping between xrootd streamids.      //
//  A single streamid can have multiple "parallel" streamids.           //
//  Their use is typically to support the client to submit multiple     //
// parallel requests (belonging to the same LogConnectionID),           //
// which are to be processed asynchronously when the answers arrive.    //
//                                                                      //
////////////////////////////////////////////////////////////////////////// 


//       $Id$


#include "XrdClient/XrdClientSid.hh"

XrdClientSid *XrdClientSid::fgInstance = 0;


XrdClientSid::XrdClientSid() {

   freesids.Resize(65536);

   // We populate the free sids queue
   for (kXR_unt16 i = 65535; i >= 1; i--)
      freesids.Push_back(i);
}

XrdClientSid::~XrdClientSid() {
   freesids.Clear();
   childsidnfo.Purge();

   delete fgInstance;
   fgInstance = 0;
}


// Gets an available sid
// From now on it will be no more available.
// A retval of 0 means that there are no more available sids
kXR_unt16 XrdClientSid::GetNewSid() {
   XrdOucMutexHelper l(fMutex);

   if (!freesids.GetSize()) return 0;
      
   return (freesids.Pop_back());

};

// Gets an available sid for a request which is to be outstanding
// This means that this sid will be inserted into the Rash
// The request gets inserted the new sid in the right place
// Also the one passed as parameter gets the new sid, as should be expected
kXR_unt16 XrdClientSid::GetNewSid(kXR_unt16 sid, ClientRequest *req) {
   XrdOucMutexHelper l(fMutex);

   if (!freesids.GetSize()) return 0;
      
   kXR_unt16 nsid = freesids.Pop_back();

   if (nsid) {
      struct SidInfo si;

      memcpy(req->header.streamid, &nsid, sizeof(req->header.streamid));

      si.fathersid = sid;
      si.outstandingreq = *req;
      si.reqbyteprogress = 0;

      childsidnfo.Add(nsid, si);
   }

      

   return nsid;

};

// Releases a sid.
// It is re-inserted into the available set
// Its info is rmeoved from the tree
void XrdClientSid::ReleaseSid(kXR_unt16 sid) {
   XrdOucMutexHelper l(fMutex);

   childsidnfo.Del(sid);
   freesids.Push_back(sid);
};


static int printoutreq(kXR_unt16,
                       struct SidInfo p, void *) {

  smartPrintClientHeader(&p.outstandingreq);
  return 0;
}
void XrdClientSid::PrintoutOutstandingRequests() {

  childsidnfo.Apply(printoutreq, this);




}
