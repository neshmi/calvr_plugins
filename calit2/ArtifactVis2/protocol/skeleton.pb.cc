// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "protocol/skeleton.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace RemoteKinect {

namespace {

const ::google::protobuf::Descriptor* Skeleton_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  Skeleton_reflection_ = NULL;
const ::google::protobuf::Descriptor* Skeleton_Joint_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  Skeleton_Joint_reflection_ = NULL;
const ::google::protobuf::EnumDescriptor* Skeleton_Joint_Type_descriptor_ = NULL;

}  // namespace


void protobuf_AssignDesc_protocol_2fskeleton_2eproto() {
  protobuf_AddDesc_protocol_2fskeleton_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "protocol/skeleton.proto");
  GOOGLE_CHECK(file != NULL);
  Skeleton_descriptor_ = file->message_type(0);
  static const int Skeleton_offsets_[4] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton, source_serial_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton, tick_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton, skeleton_id_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton, joints_),
  };
  Skeleton_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      Skeleton_descriptor_,
      Skeleton::default_instance_,
      Skeleton_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(Skeleton));
  Skeleton_Joint_descriptor_ = Skeleton_descriptor_->nested_type(0);
  static const int Skeleton_Joint_offsets_[10] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, type_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, x_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, y_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, z_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, image_x_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, image_y_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, qw_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, qx_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, qy_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, qz_),
  };
  Skeleton_Joint_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      Skeleton_Joint_descriptor_,
      Skeleton_Joint::default_instance_,
      Skeleton_Joint_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Skeleton_Joint, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(Skeleton_Joint));
  Skeleton_Joint_Type_descriptor_ = Skeleton_Joint_descriptor_->enum_type(0);
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_protocol_2fskeleton_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    Skeleton_descriptor_, &Skeleton::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    Skeleton_Joint_descriptor_, &Skeleton_Joint::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_protocol_2fskeleton_2eproto() {
  delete Skeleton::default_instance_;
  delete Skeleton_reflection_;
  delete Skeleton_Joint::default_instance_;
  delete Skeleton_Joint_reflection_;
}

void protobuf_AddDesc_protocol_2fskeleton_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\027protocol/skeleton.proto\022\014RemoteKinect\""
    "\243\005\n\010Skeleton\022\025\n\rsource_serial\030\001 \002(\t\022\014\n\004t"
    "ick\030\002 \002(\r\022\023\n\013skeleton_id\030\003 \002(\r\022,\n\006joints"
    "\030\004 \003(\0132\034.RemoteKinect.Skeleton.Joint\032\256\004\n"
    "\005Joint\022/\n\004type\030\001 \002(\0162!.RemoteKinect.Skel"
    "eton.Joint.Type\022\t\n\001x\030\002 \002(\002\022\t\n\001y\030\003 \002(\002\022\t\n"
    "\001z\030\004 \002(\002\022\017\n\007image_x\030\t \001(\002\022\017\n\007image_y\030\n \001"
    "(\002\022\n\n\002qw\030\005 \001(\002\022\n\n\002qx\030\006 \001(\002\022\n\n\002qy\030\007 \001(\002\022\n"
    "\n\002qz\030\010 \001(\002\"\200\003\n\004Type\022\010\n\004HEAD\020\001\022\010\n\004NECK\020\002\022"
    "\t\n\005TORSO\020\003\022\t\n\005WAIST\020\004\022\017\n\013COLLAR_LEFT\020\005\022\021"
    "\n\rSHOULDER_LEFT\020\006\022\016\n\nELBOW_LEFT\020\007\022\016\n\nWRI"
    "ST_LEFT\020\010\022\r\n\tHAND_LEFT\020\t\022\022\n\016FINGERTIP_LE"
    "FT\020\n\022\020\n\014COLLAR_RIGHT\020\013\022\022\n\016SHOULDER_RIGHT"
    "\020\014\022\017\n\013ELBOW_RIGHT\020\r\022\017\n\013WRIST_RIGHT\020\016\022\016\n\n"
    "HAND_RIGHT\020\017\022\023\n\017FINGERTIP_RIGHT\020\020\022\014\n\010HIP"
    "_LEFT\020\021\022\r\n\tKNEE_LEFT\020\022\022\016\n\nANKLE_LEFT\020\023\022\r"
    "\n\tFOOT_LEFT\020\024\022\r\n\tHIP_RIGHT\020\025\022\016\n\nKNEE_RIG"
    "HT\020\026\022\017\n\013ANKLE_RIGHT\020\027\022\016\n\nFOOT_RIGHT\020\030", 717);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "protocol/skeleton.proto", &protobuf_RegisterTypes);
  Skeleton::default_instance_ = new Skeleton();
  Skeleton_Joint::default_instance_ = new Skeleton_Joint();
  Skeleton::default_instance_->InitAsDefaultInstance();
  Skeleton_Joint::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_protocol_2fskeleton_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_protocol_2fskeleton_2eproto {
  StaticDescriptorInitializer_protocol_2fskeleton_2eproto() {
    protobuf_AddDesc_protocol_2fskeleton_2eproto();
  }
} static_descriptor_initializer_protocol_2fskeleton_2eproto_;


// ===================================================================

const ::google::protobuf::EnumDescriptor* Skeleton_Joint_Type_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Skeleton_Joint_Type_descriptor_;
}
bool Skeleton_Joint_Type_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const Skeleton_Joint_Type Skeleton_Joint::HEAD;
const Skeleton_Joint_Type Skeleton_Joint::NECK;
const Skeleton_Joint_Type Skeleton_Joint::TORSO;
const Skeleton_Joint_Type Skeleton_Joint::WAIST;
const Skeleton_Joint_Type Skeleton_Joint::COLLAR_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::SHOULDER_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::ELBOW_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::WRIST_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::HAND_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::FINGERTIP_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::COLLAR_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::SHOULDER_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::ELBOW_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::WRIST_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::HAND_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::FINGERTIP_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::HIP_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::KNEE_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::ANKLE_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::FOOT_LEFT;
const Skeleton_Joint_Type Skeleton_Joint::HIP_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::KNEE_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::ANKLE_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::FOOT_RIGHT;
const Skeleton_Joint_Type Skeleton_Joint::Type_MIN;
const Skeleton_Joint_Type Skeleton_Joint::Type_MAX;
const int Skeleton_Joint::Type_ARRAYSIZE;
#endif  // _MSC_VER
#ifndef _MSC_VER
const int Skeleton_Joint::kTypeFieldNumber;
const int Skeleton_Joint::kXFieldNumber;
const int Skeleton_Joint::kYFieldNumber;
const int Skeleton_Joint::kZFieldNumber;
const int Skeleton_Joint::kImageXFieldNumber;
const int Skeleton_Joint::kImageYFieldNumber;
const int Skeleton_Joint::kQwFieldNumber;
const int Skeleton_Joint::kQxFieldNumber;
const int Skeleton_Joint::kQyFieldNumber;
const int Skeleton_Joint::kQzFieldNumber;
#endif  // !_MSC_VER

Skeleton_Joint::Skeleton_Joint()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void Skeleton_Joint::InitAsDefaultInstance() {
}

Skeleton_Joint::Skeleton_Joint(const Skeleton_Joint& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void Skeleton_Joint::SharedCtor() {
  _cached_size_ = 0;
  type_ = 1;
  x_ = 0;
  y_ = 0;
  z_ = 0;
  image_x_ = 0;
  image_y_ = 0;
  qw_ = 0;
  qx_ = 0;
  qy_ = 0;
  qz_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Skeleton_Joint::~Skeleton_Joint() {
  SharedDtor();
}

void Skeleton_Joint::SharedDtor() {
  if (this != default_instance_) {
  }
}

void Skeleton_Joint::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Skeleton_Joint::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Skeleton_Joint_descriptor_;
}

const Skeleton_Joint& Skeleton_Joint::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_protocol_2fskeleton_2eproto();  return *default_instance_;
}

Skeleton_Joint* Skeleton_Joint::default_instance_ = NULL;

Skeleton_Joint* Skeleton_Joint::New() const {
  return new Skeleton_Joint;
}

void Skeleton_Joint::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    type_ = 1;
    x_ = 0;
    y_ = 0;
    z_ = 0;
    image_x_ = 0;
    image_y_ = 0;
    qw_ = 0;
    qx_ = 0;
  }
  if (_has_bits_[8 / 32] & (0xffu << (8 % 32))) {
    qy_ = 0;
    qz_ = 0;
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool Skeleton_Joint::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required .RemoteKinect.Skeleton.Joint.Type type = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::RemoteKinect::Skeleton_Joint_Type_IsValid(value)) {
            set_type(static_cast< ::RemoteKinect::Skeleton_Joint_Type >(value));
          } else {
            mutable_unknown_fields()->AddVarint(1, value);
          }
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(21)) goto parse_x;
        break;
      }
      
      // required float x = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
         parse_x:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &x_)));
          set_has_x();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(29)) goto parse_y;
        break;
      }
      
      // required float y = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
         parse_y:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &y_)));
          set_has_y();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(37)) goto parse_z;
        break;
      }
      
      // required float z = 4;
      case 4: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
         parse_z:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &z_)));
          set_has_z();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(45)) goto parse_qw;
        break;
      }
      
      // optional float qw = 5;
      case 5: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
         parse_qw:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &qw_)));
          set_has_qw();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(53)) goto parse_qx;
        break;
      }
      
      // optional float qx = 6;
      case 6: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
         parse_qx:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &qx_)));
          set_has_qx();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(61)) goto parse_qy;
        break;
      }
      
      // optional float qy = 7;
      case 7: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
         parse_qy:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &qy_)));
          set_has_qy();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(69)) goto parse_qz;
        break;
      }
      
      // optional float qz = 8;
      case 8: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
         parse_qz:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &qz_)));
          set_has_qz();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(77)) goto parse_image_x;
        break;
      }
      
      // optional float image_x = 9;
      case 9: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
         parse_image_x:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &image_x_)));
          set_has_image_x();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(85)) goto parse_image_y;
        break;
      }
      
      // optional float image_y = 10;
      case 10: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED32) {
         parse_image_y:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   float, ::google::protobuf::internal::WireFormatLite::TYPE_FLOAT>(
                 input, &image_y_)));
          set_has_image_y();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void Skeleton_Joint::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // required .RemoteKinect.Skeleton.Joint.Type type = 1;
  if (has_type()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->type(), output);
  }
  
  // required float x = 2;
  if (has_x()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(2, this->x(), output);
  }
  
  // required float y = 3;
  if (has_y()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(3, this->y(), output);
  }
  
  // required float z = 4;
  if (has_z()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(4, this->z(), output);
  }
  
  // optional float qw = 5;
  if (has_qw()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(5, this->qw(), output);
  }
  
  // optional float qx = 6;
  if (has_qx()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(6, this->qx(), output);
  }
  
  // optional float qy = 7;
  if (has_qy()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(7, this->qy(), output);
  }
  
  // optional float qz = 8;
  if (has_qz()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(8, this->qz(), output);
  }
  
  // optional float image_x = 9;
  if (has_image_x()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(9, this->image_x(), output);
  }
  
  // optional float image_y = 10;
  if (has_image_y()) {
    ::google::protobuf::internal::WireFormatLite::WriteFloat(10, this->image_y(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* Skeleton_Joint::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // required .RemoteKinect.Skeleton.Joint.Type type = 1;
  if (has_type()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      1, this->type(), target);
  }
  
  // required float x = 2;
  if (has_x()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFloatToArray(2, this->x(), target);
  }
  
  // required float y = 3;
  if (has_y()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFloatToArray(3, this->y(), target);
  }
  
  // required float z = 4;
  if (has_z()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFloatToArray(4, this->z(), target);
  }
  
  // optional float qw = 5;
  if (has_qw()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFloatToArray(5, this->qw(), target);
  }
  
  // optional float qx = 6;
  if (has_qx()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFloatToArray(6, this->qx(), target);
  }
  
  // optional float qy = 7;
  if (has_qy()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFloatToArray(7, this->qy(), target);
  }
  
  // optional float qz = 8;
  if (has_qz()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFloatToArray(8, this->qz(), target);
  }
  
  // optional float image_x = 9;
  if (has_image_x()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFloatToArray(9, this->image_x(), target);
  }
  
  // optional float image_y = 10;
  if (has_image_y()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFloatToArray(10, this->image_y(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int Skeleton_Joint::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required .RemoteKinect.Skeleton.Joint.Type type = 1;
    if (has_type()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->type());
    }
    
    // required float x = 2;
    if (has_x()) {
      total_size += 1 + 4;
    }
    
    // required float y = 3;
    if (has_y()) {
      total_size += 1 + 4;
    }
    
    // required float z = 4;
    if (has_z()) {
      total_size += 1 + 4;
    }
    
    // optional float image_x = 9;
    if (has_image_x()) {
      total_size += 1 + 4;
    }
    
    // optional float image_y = 10;
    if (has_image_y()) {
      total_size += 1 + 4;
    }
    
    // optional float qw = 5;
    if (has_qw()) {
      total_size += 1 + 4;
    }
    
    // optional float qx = 6;
    if (has_qx()) {
      total_size += 1 + 4;
    }
    
  }
  if (_has_bits_[8 / 32] & (0xffu << (8 % 32))) {
    // optional float qy = 7;
    if (has_qy()) {
      total_size += 1 + 4;
    }
    
    // optional float qz = 8;
    if (has_qz()) {
      total_size += 1 + 4;
    }
    
  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Skeleton_Joint::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const Skeleton_Joint* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const Skeleton_Joint*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void Skeleton_Joint::MergeFrom(const Skeleton_Joint& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_type()) {
      set_type(from.type());
    }
    if (from.has_x()) {
      set_x(from.x());
    }
    if (from.has_y()) {
      set_y(from.y());
    }
    if (from.has_z()) {
      set_z(from.z());
    }
    if (from.has_image_x()) {
      set_image_x(from.image_x());
    }
    if (from.has_image_y()) {
      set_image_y(from.image_y());
    }
    if (from.has_qw()) {
      set_qw(from.qw());
    }
    if (from.has_qx()) {
      set_qx(from.qx());
    }
  }
  if (from._has_bits_[8 / 32] & (0xffu << (8 % 32))) {
    if (from.has_qy()) {
      set_qy(from.qy());
    }
    if (from.has_qz()) {
      set_qz(from.qz());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void Skeleton_Joint::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Skeleton_Joint::CopyFrom(const Skeleton_Joint& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Skeleton_Joint::IsInitialized() const {
  if ((_has_bits_[0] & 0x0000000f) != 0x0000000f) return false;
  
  return true;
}

void Skeleton_Joint::Swap(Skeleton_Joint* other) {
  if (other != this) {
    std::swap(type_, other->type_);
    std::swap(x_, other->x_);
    std::swap(y_, other->y_);
    std::swap(z_, other->z_);
    std::swap(image_x_, other->image_x_);
    std::swap(image_y_, other->image_y_);
    std::swap(qw_, other->qw_);
    std::swap(qx_, other->qx_);
    std::swap(qy_, other->qy_);
    std::swap(qz_, other->qz_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata Skeleton_Joint::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = Skeleton_Joint_descriptor_;
  metadata.reflection = Skeleton_Joint_reflection_;
  return metadata;
}


// -------------------------------------------------------------------

#ifndef _MSC_VER
const int Skeleton::kSourceSerialFieldNumber;
const int Skeleton::kTickFieldNumber;
const int Skeleton::kSkeletonIdFieldNumber;
const int Skeleton::kJointsFieldNumber;
#endif  // !_MSC_VER

Skeleton::Skeleton()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void Skeleton::InitAsDefaultInstance() {
}

Skeleton::Skeleton(const Skeleton& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void Skeleton::SharedCtor() {
  _cached_size_ = 0;
  source_serial_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  tick_ = 0u;
  skeleton_id_ = 0u;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Skeleton::~Skeleton() {
  SharedDtor();
}

void Skeleton::SharedDtor() {
  if (source_serial_ != &::google::protobuf::internal::kEmptyString) {
    delete source_serial_;
  }
  if (this != default_instance_) {
  }
}

void Skeleton::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Skeleton::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Skeleton_descriptor_;
}

const Skeleton& Skeleton::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_protocol_2fskeleton_2eproto();  return *default_instance_;
}

Skeleton* Skeleton::default_instance_ = NULL;

Skeleton* Skeleton::New() const {
  return new Skeleton;
}

void Skeleton::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (has_source_serial()) {
      if (source_serial_ != &::google::protobuf::internal::kEmptyString) {
        source_serial_->clear();
      }
    }
    tick_ = 0u;
    skeleton_id_ = 0u;
  }
  joints_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool Skeleton::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required string source_serial = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_source_serial()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->source_serial().data(), this->source_serial().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_tick;
        break;
      }
      
      // required uint32 tick = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_tick:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &tick_)));
          set_has_tick();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(24)) goto parse_skeleton_id;
        break;
      }
      
      // required uint32 skeleton_id = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_skeleton_id:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &skeleton_id_)));
          set_has_skeleton_id();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(34)) goto parse_joints;
        break;
      }
      
      // repeated .RemoteKinect.Skeleton.Joint joints = 4;
      case 4: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_joints:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_joints()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(34)) goto parse_joints;
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void Skeleton::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // required string source_serial = 1;
  if (has_source_serial()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->source_serial().data(), this->source_serial().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      1, this->source_serial(), output);
  }
  
  // required uint32 tick = 2;
  if (has_tick()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(2, this->tick(), output);
  }
  
  // required uint32 skeleton_id = 3;
  if (has_skeleton_id()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(3, this->skeleton_id(), output);
  }
  
  // repeated .RemoteKinect.Skeleton.Joint joints = 4;
  for (int i = 0; i < this->joints_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      4, this->joints(i), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* Skeleton::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // required string source_serial = 1;
  if (has_source_serial()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->source_serial().data(), this->source_serial().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->source_serial(), target);
  }
  
  // required uint32 tick = 2;
  if (has_tick()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt32ToArray(2, this->tick(), target);
  }
  
  // required uint32 skeleton_id = 3;
  if (has_skeleton_id()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt32ToArray(3, this->skeleton_id(), target);
  }
  
  // repeated .RemoteKinect.Skeleton.Joint joints = 4;
  for (int i = 0; i < this->joints_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        4, this->joints(i), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int Skeleton::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required string source_serial = 1;
    if (has_source_serial()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->source_serial());
    }
    
    // required uint32 tick = 2;
    if (has_tick()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->tick());
    }
    
    // required uint32 skeleton_id = 3;
    if (has_skeleton_id()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->skeleton_id());
    }
    
  }
  // repeated .RemoteKinect.Skeleton.Joint joints = 4;
  total_size += 1 * this->joints_size();
  for (int i = 0; i < this->joints_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->joints(i));
  }
  
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Skeleton::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const Skeleton* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const Skeleton*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void Skeleton::MergeFrom(const Skeleton& from) {
  GOOGLE_CHECK_NE(&from, this);
  joints_.MergeFrom(from.joints_);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_source_serial()) {
      set_source_serial(from.source_serial());
    }
    if (from.has_tick()) {
      set_tick(from.tick());
    }
    if (from.has_skeleton_id()) {
      set_skeleton_id(from.skeleton_id());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void Skeleton::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Skeleton::CopyFrom(const Skeleton& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Skeleton::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000007) != 0x00000007) return false;
  
  for (int i = 0; i < joints_size(); i++) {
    if (!this->joints(i).IsInitialized()) return false;
  }
  return true;
}

void Skeleton::Swap(Skeleton* other) {
  if (other != this) {
    std::swap(source_serial_, other->source_serial_);
    std::swap(tick_, other->tick_);
    std::swap(skeleton_id_, other->skeleton_id_);
    joints_.Swap(&other->joints_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata Skeleton::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = Skeleton_descriptor_;
  metadata.reflection = Skeleton_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace RemoteKinect

// @@protoc_insertion_point(global_scope)