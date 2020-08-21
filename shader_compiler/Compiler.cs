using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Text;
using System.Diagnostics;

namespace shader_compiler
{
    public class Compiler
    {
        private const string psDebug = "-T ps_6_5 -D HLSL -Od -Zi -Qembed_debug -E {0} {1} -Fo {2}";
        private const string psRelease = "-T ps_6_5 -D HLSL -E {0} {1} -Fo {2}";
        private const string rsDebug = "-T cs_6_5 -D HLSL -Od -Zi -Qembed_debug -rootsig-define RS -extractrootsignature {0} -Fo {1}";
        private const string rsRelease = "-T cs_6_5 -D HLSL -rootsig-define RS {0} -Fo {1}";
        private const string vsDebug = "-T vs_6_5 -D HLSL -Od -Zi -Qembed_debug -E {0} {1} -Fo {2}";
        private const string vsRelease = "-T vs_6_5 -D HLSL -E {0} {1} -Fo {2}";
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
                foreach (var componment in pipelineState.components_)
                {
                    var cmd = string.Empty;

                    switch (componment.type_)
                    {
                        case ShaderType.RootSignature:
                            if (options_.IsDebug)
                                cmd = string.Format(rsDebug, componment.path_, $"{pipelineState.name_}.rso");
                            else
                                cmd = string.Format(rsRelease, componment.path_, $"{pipelineState.name_}.rso");
                            break;

                        case ShaderType.VertexShader:
                            if (options_.IsDebug)
                                cmd = string.Format(vsDebug, componment.entry_, componment.path_, $"{pipelineState.name_}.vso");
                            else
                                cmd = string.Format(vsRelease, componment.entry_, componment.path_, $"{pipelineState.name_}.vso");
                            break;

                        case ShaderType.PixelShader:
                            if (options_.IsDebug)
                                cmd = string.Format(psDebug, componment.entry_, componment.path_, $"{pipelineState.name_}.pso");
                            else
                                cmd = string.Format(psRelease, componment.entry_, componment.path_, $"{pipelineState.name_}.pso");
                            break;

                        default:
                            throw new System.InvalidOperationException("Unknown shader type");
                    }

                    var process = new Process();

                    process.StartInfo.FileName = options_.CompilerPath;
                    process.StartInfo.WorkingDirectory = options_.OutDir;
                    process.StartInfo.Arguments = cmd;

                    process.Start();
                }
            }
        }

        #endregion Public Methods
    }
}