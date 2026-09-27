'use strict';
// The first compressed SYWP profile accepts only a constant-rate MP4V video
// track. Walk box boundaries rather than searching for atom names in media data.
function inspect(bytes){
 if(!Buffer.isBuffer(bytes)||bytes.length<32)throw Error('Invalid MP4 payload');
 const u32=(at,end=bytes.length)=>{if(at<0||at+4>end)throw Error('Truncated MP4 field');return bytes.readUInt32BE(at);};
 function boxes(start,end){const found=[];for(let at=start;at<end;){if(end-at<8)throw Error('Truncated MP4 box');const length=u32(at,end),type=bytes.toString('ascii',at+4,at+8);if(length<8||length>end-at)throw Error('Invalid MP4 box extent');found.push({at,end:at+length,type});at+=length;}return found;}
 const one=(list,type)=>{const matches=list.filter(x=>x.type===type);if(matches.length!==1)throw Error('Expected one MP4 '+type+' box');return matches[0];};
 const child=(parent,type)=>one(boxes(parent.at+8,parent.end),type);
 const root=boxes(0,bytes.length),ftyp=one(root,'ftyp'),moov=one(root,'moov'),mdat=one(root,'mdat');
 if(ftyp.at!==0||ftyp.end-ftyp.at<16||mdat.end-mdat.at<=8)throw Error('Invalid MP4 media layout');
 const traks=boxes(moov.at+8,moov.end).filter(x=>x.type==='trak');if(traks.length!==1)throw Error('MP4 must contain one video track');
 const trak=traks[0],tkhd=child(trak,'tkhd'),mdia=child(trak,'mdia'),hdlr=child(mdia,'hdlr'),mdhd=child(mdia,'mdhd');
 if(hdlr.end-hdlr.at<20||bytes.toString('ascii',hdlr.at+16,hdlr.at+20)!=='vide'||tkhd.end-tkhd.at<84||bytes[tkhd.at+8]!==0||bytes[mdhd.at+8]!==0||mdhd.end-mdhd.at<32)throw Error('Unsupported MP4 track');
 const width=u32(tkhd.end-8)>>>16,height=u32(tkhd.end-4)>>>16,timescale=u32(mdhd.at+20);
 const stbl=child(child(mdia,'minf'),'stbl'),stsd=child(stbl,'stsd'),stsz=child(stbl,'stsz'),stts=child(stbl,'stts');
 if(stsd.end-stsd.at<24||u32(stsd.at+12)!==1||bytes.toString('ascii',stsd.at+20,stsd.at+24)!=='mp4v')throw Error('MP4V video required');
 if(stsz.end-stsz.at<20||stts.end-stts.at!==24||u32(stts.at+12)!==1)throw Error('Unsupported MP4 sample table');
 const frames=u32(stsz.at+16),timedFrames=u32(stts.at+16),delta=u32(stts.at+20);
 if(!timescale||!delta||!frames||frames!==timedFrames||frames>100000||!width||!height)throw Error('Invalid MP4 timing or dimensions');
 const gcd=(a,b)=>b?gcd(b,a%b):a,g=gcd(timescale,delta);
 return {codec:'mpeg4-part2',width,height,frames,fpsNumerator:timescale/g,fpsDenominator:delta/g};
}
module.exports={inspect};
