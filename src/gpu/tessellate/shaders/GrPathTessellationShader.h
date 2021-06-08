/*
 * Copyright 2019 Google LLC.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrPathTessellationShader_DEFINED
#define GrPathTessellationShader_DEFINED

#include "src/gpu/glsl/GrGLSLGeometryProcessor.h"
#include "src/gpu/tessellate/GrTessellationPathRenderer.h"
#include "src/gpu/tessellate/shaders/GrTessellationShader.h"

// This is the base class for shaders in the GPU tessellator that fill paths.
class GrPathTessellationShader : public GrTessellationShader {
public:
    // Draws a simple array of triangles.
    static GrPathTessellationShader* MakeSimpleTriangleShader(SkArenaAlloc*,
                                                              const SkMatrix& viewMatrix,
                                                              const SkPMColor4f&);

    // Uses instanced draws to triangulate standalone closed curves with a "middle-out" topology.
    // Middle-out draws a triangle with vertices at T=[0, 1/2, 1] and then recurses breadth first:
    //
    //   depth=0: T=[0, 1/2, 1]
    //   depth=1: T=[0, 1/4, 2/4], T=[2/4, 3/4, 1]
    //   depth=2: T=[0, 1/8, 2/8], T=[2/8, 3/8, 4/8], T=[4/8, 5/8, 6/8], T=[6/8, 7/8, 1]
    //   ...
    //
    // The caller may compute each cubic's resolveLevel on the CPU (i.e., the log2 number of line
    // segments it will be divided into; see GrWangsFormula::cubic_log2/quadratic_log2/conic_log2),
    // and then sort the instance buffer by resolveLevel for efficient batching of indirect draws.
    static GrPathTessellationShader* MakeMiddleOutInstancedShader(SkArenaAlloc*,
                                                                  const SkMatrix& viewMatrix,
                                                                  const SkPMColor4f&);

    // Uses GPU tessellation shaders to linearize, triangulate, and render cubic "wedge" patches. A
    // wedge is a 5-point patch consisting of 4 cubic control points, plus an anchor point fanning
    // from the center of the curve's resident contour.
    static GrPathTessellationShader* MakeHardwareWedgeShader(SkArenaAlloc*,
                                                             const SkMatrix& viewMatrix,
                                                             const SkPMColor4f&);

    // Uses GPU tessellation shaders to linearize, triangulate, and render standalone closed cubics.
    static GrPathTessellationShader* MakeHardwareCurveShader(SkArenaAlloc*,
                                                             const SkMatrix& viewMatrix,
                                                             const SkPMColor4f&);

    // Returns the stencil settings to use for a standard Redbook "stencil" pass.
    static const GrUserStencilSettings* StencilPathSettings(SkPathFillType fillType) {
        // Increments clockwise triangles and decrements counterclockwise. Used for "winding" fill.
        constexpr static GrUserStencilSettings kIncrDecrStencil(
            GrUserStencilSettings::StaticInitSeparate<
                0x0000,                                0x0000,
                GrUserStencilTest::kAlwaysIfInClip,    GrUserStencilTest::kAlwaysIfInClip,
                0xffff,                                0xffff,
                GrUserStencilOp::kIncWrap,             GrUserStencilOp::kDecWrap,
                GrUserStencilOp::kKeep,                GrUserStencilOp::kKeep,
                0xffff,                                0xffff>());

        // Inverts the bottom stencil bit. Used for "even/odd" fill.
        constexpr static GrUserStencilSettings kInvertStencil(
            GrUserStencilSettings::StaticInit<
                0x0000,
                GrUserStencilTest::kAlwaysIfInClip,
                0xffff,
                GrUserStencilOp::kInvert,
                GrUserStencilOp::kKeep,
                0x0001>());

        SkASSERT(fillType == SkPathFillType::kWinding || fillType == SkPathFillType::kEvenOdd);
        return (fillType == SkPathFillType::kWinding) ? &kIncrDecrStencil : &kInvertStencil;
    }

    // Returns the stencil settings to use for a standard Redbook "fill" pass. Allows non-zero
    // stencil values to pass and write a color, and resets the stencil value back to zero; discards
    // immediately on stencil values of zero.
    static const GrUserStencilSettings* TestAndResetStencilSettings() {
        constexpr static GrUserStencilSettings kTestAndResetStencil(
            GrUserStencilSettings::StaticInit<
                0x0000,
                // No need to check the clip because the previous stencil pass will have only
                // written to samples already inside the clip.
                GrUserStencilTest::kNotEqual,
                0xffff,
                GrUserStencilOp::kZero,
                GrUserStencilOp::kKeep,
                0xffff>());
        return &kTestAndResetStencil;
    }

    // Creates a pipeline that does not write to the color buffer.
    static const GrPipeline* MakeStencilOnlyPipeline(
            const ProgramArgs& args, GrAAType aaType,
            GrTessellationPathRenderer::PathFlags pathFlags, const GrAppliedHardClip& hardClip) {
        using PathFlags = GrTessellationPathRenderer::PathFlags;
        GrPipeline::InitArgs pipelineArgs;
        if (aaType == GrAAType::kMSAA) {
            pipelineArgs.fInputFlags |= GrPipeline::InputFlags::kHWAntialias;
        }
        if (args.fCaps->wireframeSupport() && (pathFlags & PathFlags::kWireframe)) {
            pipelineArgs.fInputFlags |= GrPipeline::InputFlags::kWireframe;
        }
        pipelineArgs.fCaps = args.fCaps;
        return args.fArena->make<GrPipeline>(pipelineArgs,
                                             GrDisableColorXPFactory::MakeXferProcessor(),
                                             hardClip);
    }

protected:
    GrPathTessellationShader(ClassID classID, GrPrimitiveType primitiveType,
                             int tessellationPatchVertexCount, const SkMatrix& viewMatrix,
                             const SkPMColor4f& color)
            : GrTessellationShader(classID, primitiveType, tessellationPatchVertexCount, viewMatrix,
                                   color) {
    }

    // Default path tessellation shader implementation that manages a uniform matrix and color.
    class Impl : public GrGLSLGeometryProcessor {
    public:
        void onEmitCode(EmitArgs&, GrGPArgs*) final;
        void setData(const GrGLSLProgramDataManager&, const GrShaderCaps&,
                     const GrGeometryProcessor&) override;

    protected:
        // float2 eval_rational_cubic(float4x3 P, float T) { ...
        //
        // Converts a 4-point input patch into the rational cubic it intended to represent.
        static const char* kUnpackRationalCubicFn;

        // float4x3 unpack_rational_cubic(float2 p0, float2 p1, float2 p2, float2 p3) { ...
        //
        // Evaluate our point of interest using numerically stable linear interpolations. We add our
        // own "safe_mix" method to guarantee we get exactly "b" when T=1. The builtin mix()
        // function seems spec'd to behave this way, but empirical results results have shown it
        // does not always.
        static const char* kEvalRationalCubicFn;

        virtual void emitVertexCode(GrGLSLVertexBuilder*, GrGPArgs*) = 0;

        GrGLSLUniformHandler::UniformHandle fAffineMatrixUniform;
        GrGLSLUniformHandler::UniformHandle fTranslateUniform;
        GrGLSLUniformHandler::UniformHandle fColorUniform;
    };
};

#endif
