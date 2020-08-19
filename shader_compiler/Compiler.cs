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
        private const string rsDebug = "-T cs_6_5 -Od -Zi -Qembed_debug -rootsig-define RS -extractrootsignature {0} -Fo {1}";

        private Options options_;

        #region Public Methods

        public Compiler(Options o)
        {
            options_ = o;
        }

        public void CompileShaders(List<Shader> shaders)
        {
            foreach (var shader in shaders)
            {
                var cmd = string.Empty;

                switch (shader.type_)
                {
                    case ShaderType.RootSignature:
                        if (options_.IsDebug)
                            cmd = string.Format(rsDebug, shader.path_, $"{shader.name_}.rso");
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

        #endregion Public Methods
    }
}