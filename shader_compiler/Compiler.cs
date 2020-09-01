using System.Collections.Generic;
using System.Diagnostics;

namespace shader_compiler
{
    public class Compiler
    {
        // WARP limits us to 6.4
        private const string psDebug = "-T ps_6_4 -D HLSL -Od -Zi -Qembed_debug -E {0} {1} -Fo {2}";

        private const string psRelease = "-T ps_6_4 -D HLSL -E {0} {1} -Fo {2}";
        private const string psVerify = "-T ps_6_4 -verifyrootsignature {0} {1}";
        private const string rsDebug = "-T cs_6_4 -D HLSL -Od -Zi -Qembed_debug -rootsig-define RS -extractrootsignature {0} -Fo {1}";
        private const string rsRelease = "-T cs_6_4 -D HLSL -rootsig-define RS {0} -Fo {1}";
        private const string vsDebug = "-T vs_6_4 -D HLSL -Od -Zi -Qembed_debug -E {0} {1} -Fo {2}";
        private const string vsRelease = "-T vs_6_4 -D HLSL -E {0} {1} -Fo {2}";
        private const string vsVerify = "-T vs_6_4 -verifyrootsignature {0} {1}";
        private Options options_;

        #region Public Methods

        public Compiler(Options o)
        {
            options_ = o;
        }

        public void CompileShaders(List<PipelineState> pipelineStates)
        {
            foreach (var pipelineState in pipelineStates)
            {
                foreach (var kvp in pipelineState.components_)
                {
                    var cmd = string.Empty;

                    switch (kvp.Value.type_)
                    {
                        case ShaderType.RootSignature:
                            if (options_.IsDebug)
                                cmd = string.Format(rsDebug, kvp.Value.path_, $"{pipelineState.name_}_rs.dxco");
                            else
                                cmd = string.Format(rsRelease, kvp.Value.path_, $"{pipelineState.name_}_rs.dxco");
                            break;

                        case ShaderType.PixelShader:
                            if (options_.IsDebug)
                                cmd = string.Format(psDebug, kvp.Value.entry_, kvp.Value.path_, $"{pipelineState.name_}_ps.dxco");
                            else
                                cmd = string.Format(psRelease, kvp.Value.entry_, kvp.Value.path_, $"{pipelineState.name_}_ps.dxco");
                            break;

                        case ShaderType.VertexShader:
                            if (options_.IsDebug)
                                cmd = string.Format(vsDebug, kvp.Value.entry_, kvp.Value.path_, $"{pipelineState.name_}_vs.dxco");
                            else
                                cmd = string.Format(vsRelease, kvp.Value.entry_, kvp.Value.path_, $"{pipelineState.name_}_vs.dxco");
                            break;

                        default:
                            throw new System.InvalidOperationException("Unknown shader type");
                    }

                    var process = new Process();

                    process.StartInfo.FileName = options_.CompilerPath;
                    process.StartInfo.WorkingDirectory = options_.OutDir;
                    process.StartInfo.Arguments = cmd;

                    process.Start();

                    // validate shader against root signature
                    // seems broken so disable for now
                    //var validate = false;

                    //switch (kvp.Value.type_)
                    //{
                    //    case ShaderType.PixelShader:
                    //        cmd = string.Format(psVerify, $"{pipelineState.name_}_rs.dxco", $"{pipelineState.name_}_ps.dxco");
                    //        validate = true;
                    //        break;

                    //    case ShaderType.VertexShader:
                    //        cmd = string.Format(vsVerify, $"{pipelineState.name_}_rs.dxco", $"{pipelineState.name_}_vs.dxco");
                    //        validate = true;
                    //        break;
                    //}

                    //if (validate)
                    //{
                    //    process = new Process();

                    //    process.StartInfo.FileName = options_.CompilerPath;
                    //    process.StartInfo.WorkingDirectory = options_.OutDir;
                    //    process.StartInfo.Arguments = cmd;

                    //    process.Start();
                    //}
                }
            }
        }

        #endregion Public Methods
    }
}