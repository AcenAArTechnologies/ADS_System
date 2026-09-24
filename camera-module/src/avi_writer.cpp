#include "avi_writer.h"
#include <SD_MMC.h>

static void writeU32(File &f, uint32_t v) { f.write((const uint8_t *)&v, 4); }
static void writeU16(File &f, uint16_t v) { f.write((const uint8_t *)&v, 2); }
static void writeTag(File &f, const char tag[4]) { f.write((const uint8_t *)tag, 4); }

bool AviWriter::begin(const String &path, uint16_t width, uint16_t height, uint8_t fps) {
  width_ = width;
  height_ = height;
  fps_ = fps;
  frameCount_ = 0;
  frameSizes_.clear();

  file_ = SD_MMC.open(path, FILE_WRITE);
  if (!file_) return false;

  // RIFF header (size backfilled in finalize())
  writeTag(file_, "RIFF");
  writeU32(file_, 0);
  writeTag(file_, "AVI ");

  // hdrl LIST
  writeTag(file_, "LIST");
  writeU32(file_, 4 + 8 + 56 + 8 + 8 + 56 + 8 + 40); // hdrl list size
  writeTag(file_, "hdrl");

  // avih (main AVI header)
  writeTag(file_, "avih");
  writeU32(file_, 56);
  uint32_t usPerFrame = 1000000UL / fps_;
  writeU32(file_, usPerFrame);
  writeU32(file_, 0);           // max bytes per sec
  writeU32(file_, 0);           // padding granularity
  writeU32(file_, 0x10);        // flags: AVIF_HASINDEX
  uint32_t totalFramesOffset = file_.position();
  writeU32(file_, 0);           // total frames (backfilled)
  writeU32(file_, 0);           // initial frames
  writeU32(file_, 1);           // streams
  writeU32(file_, 0);           // suggested buffer size
  writeU32(file_, width_);
  writeU32(file_, height_);
  writeU32(file_, 0); writeU32(file_, 0); writeU32(file_, 0); writeU32(file_, 0); // reserved

  // strl LIST
  writeTag(file_, "LIST");
  writeU32(file_, 4 + 8 + 56 + 8 + 40);
  writeTag(file_, "strl");

  // strh (stream header)
  writeTag(file_, "strh");
  writeU32(file_, 56);
  writeTag(file_, "vids");
  writeTag(file_, "MJPG");
  writeU32(file_, 0);  // flags
  writeU16(file_, 0);  // priority
  writeU16(file_, 0);  // language
  writeU32(file_, 0);  // initial frames
  writeU32(file_, 1);  // scale
  writeU32(file_, fps_); // rate (rate/scale = fps)
  writeU32(file_, 0);  // start
  uint32_t strhLengthOffset = file_.position();
  writeU32(file_, 0);  // length (frame count, backfilled)
  writeU32(file_, 0);  // suggested buffer size
  writeU32(file_, 0xFFFFFFFF); // quality
  writeU32(file_, 0);  // sample size
  writeU16(file_, 0); writeU16(file_, 0); writeU16(file_, (uint16_t)width_); writeU16(file_, (uint16_t)height_); // frame rect

  // strf (BITMAPINFOHEADER)
  writeTag(file_, "strf");
  writeU32(file_, 40);
  writeU32(file_, 40); // biSize
  writeU32(file_, width_);
  writeU32(file_, height_);
  writeU16(file_, 1);  // planes
  writeU16(file_, 24); // bit count
  writeTag(file_, "MJPG"); // compression
  writeU32(file_, width_ * height_ * 3);
  writeU32(file_, 0); writeU32(file_, 0); writeU32(file_, 0); writeU32(file_, 0);

  // movi LIST (size backfilled)
  writeTag(file_, "LIST");
  moviSizeOffset_ = file_.position();
  writeU32(file_, 0);
  writeTag(file_, "movi");
  moviDataStart_ = file_.position();

  (void)totalFramesOffset;
  (void)strhLengthOffset;
  return true;
}

bool AviWriter::addFrame(const uint8_t *jpeg, size_t len) {
  if (!file_) return false;
  writeTag(file_, "00dc");
  writeU32(file_, (uint32_t)len);
  file_.write(jpeg, len);
  if (len % 2 != 0) {
    uint8_t pad = 0;
    file_.write(&pad, 1); // chunks are word-aligned
  }
  frameSizes_.push_back((uint32_t)len);
  frameCount_++;
  return true;
}

void AviWriter::finalize() {
  if (!file_) return;

  uint32_t idx1Start = file_.position();
  writeTag(file_, "idx1");
  writeU32(file_, frameCount_ * 16);
  uint32_t offset = 4; // relative to movi data, first chunk starts right after "movi" tag
  for (uint32_t sz : frameSizes_) {
    writeTag(file_, "00dc");
    writeU32(file_, 0x10); // AVIIF_KEYFRAME
    writeU32(file_, offset);
    writeU32(file_, sz);
    offset += 8 + sz + (sz % 2);
  }

  uint32_t fileEnd = file_.position();

  // Backfill RIFF size.
  file_.seek(4);
  writeU32(file_, fileEnd - 8);

  // Backfill total frames in avih (offset 48 from file start: 4 RIFF tag + 4 size + 4 'AVI ' + 4 LIST + 4 size + 4 'hdrl' + 4 'avih' + 4 size + 4 usPerFrame + 4 maxBytes + 4 padding + 4 flags = byte 48)
  file_.seek(48);
  writeU32(file_, frameCount_);

  // Backfill strh length (frame count) - offset computed from structure above.
  file_.seek(48 + 4 /*init frames*/ + 4 /*streams*/ + 4 /*sugg buf*/ + 4 /*w*/ + 4 /*h*/ + 16 /*reserved*/
              + 8 /*LIST size*/ + 4 /*strl*/ + 8 /*strh tag+size*/ + 4 /*fccType*/ + 4 /*fccHandler*/
              + 4 /*flags*/ + 2 /*priority*/ + 2 /*language*/ + 4 /*init frames*/ + 4 /*scale*/ + 4 /*rate*/ + 4 /*start*/);
  writeU32(file_, frameCount_);

  // Backfill movi LIST size: "movi" tag (4 bytes) + all chunk data up to idx1.
  file_.seek(moviSizeOffset_);
  writeU32(file_, 4 + (idx1Start - moviDataStart_));

  file_.close();
}
