/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_C_SERVICE_STUB_CODEE_MITTER_H
#define OHOS_HDI_C_SERVICE_STUB_CODEE_MITTER_H

#include "codegen/c_code_emitter.h"

namespace OHOS {
namespace HDI {
class CServiceStubCodeEmitter : public CCodeEmitter {
public:
    CServiceStubCodeEmitter() : CCodeEmitter() {}

    virtual ~CServiceStubCodeEmitter() = default;
private:
    bool ResolveDirectory(const String& targetDirectory) override;

    void EmitCode() override;

    void EmitCbServiceStubHeaderFile();

    void EmitCbServiceStubMethodsDcl(StringBuilder& sb);

    void EmitServiceStubSourceFile();

    void EmitServiceStubInclusions(StringBuilder& sb);

    void EmitServiceStubStdlibInclusions(StringBuilder& sb);

    void EmitDriverLibPath(StringBuilder& sb);

    void EmitServConstructTypedef(StringBuilder& sb);

    void EmitServiceStubMethodImpls(StringBuilder& sb, const String& prefix);

    void EmitServiceStubMethodImpl(const AutoPtr<ASTMethod>& method, StringBuilder& sb, const String& prefix);

    void EmitStubLocalVariable(const AutoPtr<ASTParameter>& param, StringBuilder& sb, const String& prefix);

    void EmitReadStubMethodParameter(const AutoPtr<ASTParameter>& param, const String& parcelName, StringBuilder& sb,
        const String& prefix);

    void EmitReadCStringStubMethodParameter(const AutoPtr<ASTParameter>& param, const String& parcelName,
        StringBuilder& sb, const String& prefix, AutoPtr<ASTType>& type);

    void EmitStubCallMethod(const AutoPtr<ASTMethod>& method, const String& gotoLabel, StringBuilder& sb,
        const String& prefix);

    void EmitCallParameter(StringBuilder& sb, const AutoPtr<ASTType>& type, ParamAttr attribute, const String& name);

    void EmitServiceStubOnRequestMethodImpl(StringBuilder& sb, const String& prefix);

    void EmitCbStubDefinitions(StringBuilder& sb);

    void EmitStubGetMethodImpl(StringBuilder& sb);

    void EmitKernelStubGetMethodImpl(StringBuilder& sb);

    void EmitStubLinkService(StringBuilder& sb);

    void EmitStubReleaseImpl(StringBuilder& sb);

    void EmitKernelStubReleaseImpl(StringBuilder& sb);
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_C_SERVICE_STUB_CODEE_MITTER_H