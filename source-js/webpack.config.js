const HtmlWebpackPlugin = require('html-webpack-plugin');
const path = require('path');

module.exports = (_, argv) => ({
    entry: './render/index.tsx',
    target: 'electron-renderer',
    output: {
        filename: 'main.js',
        path: path.resolve(__dirname, 'build'),
    },
    resolve: {
        extensions: ['.tsx', '.ts', '.js'],
    },
    module: {
        rules: [
            {
                test: /\.tsx?$/,
                exclude: /(node_modules|bower_components)/,
                use: {
                    loader: 'swc-loader',
                    options: {
                        jsc: {
                            parser: {
                                syntax: 'typescript',
                                tsx: true,
                            },
                            transform: {
                                react: {
                                    pragma: 'React.createElement',
                                    pragmaFrag: 'React.Fragment',
                                    throwIfNamespace: true,
                                    development: false,
                                    useBuiltins: false,
                                    runtime: 'automatic',
                                },
                            },
                            target: 'es2020',
                            minify:
                                argv.mode === 'production'
                                    ? { compress: true, mangle: true }
                                    : undefined,
                        },
                    },
                },
            },
            {
                test: /\.css$/i,
                use: ['style-loader', 'css-loader'],
            },
            {
                test: /\.svg$/,
                use: ['@svgr/webpack'],
            },
            {
                test: /\.(ico|svg|png|jpg|gif|webp|wasm)$/i,
                use: [
                    {
                        loader: 'url-loader',
                        options: {
                            limit: 8192,
                        },
                    },
                ],
            },
            {
                test: /\.glsl$/,
                type: 'asset/source',
            },
        ],
    },
    plugins: [
        new HtmlWebpackPlugin({
            template: path.resolve(__dirname, 'render', 'template', 'index.html'),
            filename: 'index.html',
        }),
    ],
});
