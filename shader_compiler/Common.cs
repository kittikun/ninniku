using CommandLine;
using System.Collections.Generic;

namespace shader_compiler
{
    public enum ShaderType
    {
        RootSignature,
        VertexShader,
        PixelShader
    }

    public struct ShaderComponent
    {
        public string entry_;
        public string path_;
        public ShaderType type_;
    }

    public class Options
    {
        [Option('d', "dxcPath", Required = true, HelpText = "Path to dxc.exe")]
        public string CompilerPath { get; set; }

        [Option('c', "config", Required = true, HelpText = "Configuration (Debug/Release) to build")]
        public string Configuration { get; set; }

        [Option('i', "input", Required = true, HelpText = "Input shaders TOC file")]
        public string InputPath { get; set; }

        public bool IsDebug { get; set; }

        [Option('o', "outDir", Required = true, HelpText = "Output folder")]
        public string OutDir { get; set; }
    }

    public class PipelineState
    {
        public List<ShaderComponent> components_ = new List<ShaderComponent>();
        public string name_;
    }
}