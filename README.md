# arHttp: Offloads USD asset resolution to an HTTP server

**Status**: Proof Of Concept

## Intention

[OpenUSD](https://openusd.org) does not support [custom resolvers](https://openusd.org/release/api/ar_page_front.html) implemented in Python to avoid performance issues, especially for multi-threaded consumers. It however could be valuable to prototype a resolver in Python, another language or to offload asset resolving to a central network service for ease of distribution and maintenance.

`arHttp` is a proof of concept for a simple resolver written in C++ that queries an HTTP server to do the actual resolving: the requested asset path is sent to configurable HTTP endpoint and the response is the resolved asset path. This code is currently intented to be a starting point for anyone looking at implementing such a resolver in a production environment. It is also a learning experience for me and a way to discover the `arResolver` API.

A [sample web application is included in this repo](src/arHttpSampleServer/__init__.py) to be used as a basis to prototype a resolver in Python 3.8+:

```python

import pathlib

from fastapi import FastAPI, HTTPException
from fastapi.responses import PlainTextResponse

_layersdir = pathlib.Path(__file__).parent

_ID_LAYERPATH_MAP = {
    "id0x01": str(_layersdir / "layer1.usda"),
    "id0x02": str(_layersdir / "layer2.usda"),
}

app = FastAPI()

@app.get("/{asset_path}", response_class=PlainTextResponse)
async def root(asset_path):
    if resolved_path := _ID_LAYERPATH_MAP.get(asset_path):
        return resolved_path
    raise HTTPException(status_code=404)
```

### Protocol

`arHttp` makes request to an HTTP server. The request verb **MUST** be `GET`. The requested path **MUST** be part of the queried URL. Is the requested path can be resolved the server responds with [a status code `200 OK`](https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/200) and the resolved path in the body of the response.

For example in the following request / response, the asset path `id0x01` is resolved to `/home/charles/src/usdArHttp/src/arHttpSampleServer/layer1.usda`:

```
# Request

GET /id0x01 HTTP/1.1
Host: localhost:8000
Accept: */*

```

```
# Response

HTTP/1.1 200 OK
content-length: 62
content-type: text/plain; charset=utf-8

/home/charles/src/usdArHttp/src/arHttpSampleServer/layer1.usda
```

If the server respond with a code different of status `200 OK`, the response is ignored and `arHttp` returns an empty path.

```
# Request

GET /id0xThisIsIsUnknown HTTP/1.1
Host: localhost:8000
User-Agent: curl/7.88.1
Accept: */*

```

```
# Response

HTTP/1.1 404 Not Found
content-length: 0
content-type: text/plain; charset=utf-8


```

### HTTP endpoint configuration

The default endpoint is `http://localhost:8000/%s`, where `%s` is replaced by the requested asset path. For example, an asset path named `models/3a20efb0-76fa-4a76-8e49-24c46ecfd87c` will `GET http://localhost:8000/models/3a20efb0-76fa-4a76-8e49-24c46ecfd87c`.

Both the server and URL path can be configured by environment variables:
* **AR_HTTP_SERVER_URL** (default: `http://localhost:8000`) is the arHttp server
* **AR_HTTP_PATH_FORMAT** (default: `/%s`) is the arHttp url format. A single `%s` is required here.

### Debugging

`arHttp` defines the `AR_HTTP` logger. To display runtime debugging messages add `AR_HTTP` to the list of logger in the environment variables. For example, on Linux:

```sh
export TF_DEBUG=AR_HTTP
```

### Resolving sequence

To avoid impairing performances too much during development, `arHttp` first calls the default resolver and queries the HTTP server if no asset has been found.

```mermaid
sequenceDiagram
    USD->>arHttp: request assetPath
    arHttp->>arDefaultResolver: request assetPath
    alt resolved
        arDefaultResolver->>arHttp: not empty resolved path
    else not resolved
        arDefaultResolver->>arHttp: empty resolved path
        arHttp->>HTTP Server: HTTP GET assetPath
        HTTP Server->>arHttp: HTTP response resolved path
    end
    arHttp->>USD: resolved path
```

### Topics to explore

* **arResolver contexts**: `arHttp` does not handle [`ArResolver`'s contexts yet](https://openusd.org/release/api/class_ar_resolver_context.html)
* **authentification**: there is currently no possiblity to authenticate the HTTP queries. HTTP authentification is usually done with special headers like [`Authorization`](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Authorization). Authentification could be implemented with custom header defined in environment variables. For example, to implement the [simple `Basic` method](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Authorization#basic_authentication), an environment could set the variable `AR_HTTP_HEADER_Authorization="Basic YWxhZGRpbjpvcGVuc2VzYW1l"`.
* **heavily multithreaded resolving**: `arHttp` has not been tested on scenes with large numbers of concurrent asset resolving

## Building and running

### Requirements

* [cmake]() >= 3.14
* `arHttp` has been built on Linux only, but *should* work on other platforms
* `arHttp` has been built against USD 23.08 but *should* work on any USD version with `Ar v2.0` (USD >= 21.02)

### Building `arHttp`

As `OpenUSD`, `arHttp` is built with `cmake`. The only `arHttp` specific variable is `PXR_CONFIG_CMAKE`: it is expected to be set to the location of the `pxrConfig.cmake` that comes with OpenUSD's binaries.

A typical build sequence that would install `arHttp` to the `~/opt/arHttp` folder would be:

```sh
git clone https://github.com/charlesfleche/arHttp.git
mkdir build-arHttp
cd build-arHttp
cmake -DCMAKE_INSTALL_PREFIX=~/opt/arHttp -DPXR_CONFIG_CMAKE=~/opt/OpenUSD-v23.08/pxrConfig.cmake ../arHttp
cmake --build . --target install
```

The folder `~/opt/arHttp` should now contain the plugin shared library `arHttp.so` and the associated plugin configuration file `plugInfo.json`:

```sh
tree ~/opt/arHttp

~/opt/arHttp/
├── arHttp
│   └── resources
│       └── plugInfo.json
└── arHttp.so
```

### Running `arHttp`

Add the path to the `arHttp`'s `plugInfo.json` folder to OpenUSD's environment variable `PXR_PLUGINPATH_NAME`.

```sh
export PXR_PLUGINPATH_NAME=/home/charles/opt/arHttp/arHttp/resources
```

### Python sample resolver web application

### Installation

The sample resolver web application is installable with its dependencies in a standard Python virtual environment:

```sh
python3 -m venv venv
source venv/bin/activate
python -m pip install -e .
```

### Running

The sample http server uses the [FastAPI](https://fastapi.tiangolo.com/) web framework and as such is ran inside an [ASGI](https://asgi.readthedocs.io/en/latest/) server implementation like [uvicorn](https://www.uvicorn.org/) (installed as a dependency alongside the sample web server).

```
# This run the sampler web server for local hosts only on port 8000.
# The web application will reload after a code change.

uvicorn arHttpSampleServer:app --reload

INFO:     Will watch for changes in these directories: ['/home/charles/src/usdArHttp']
INFO:     Uvicorn running on http://127.0.0.1:8000 (Press CTRL+C to quit)
INFO:     Started reloader process [128622] using WatchFiles
INFO:     Started server process [128624]
INFO:     Waiting for application startup.
INFO:     Application startup complete.
```

### Testing

After building `arHttp`, setting the `PXR_PLUGINPATH_NAME` environment variable and running the sample web server, it's now possible to resolve the sample scene.

```
#usda 1.0
(
    subLayers = [
        @id0x01@,
        @id0x02@
    ]
)
```

Flattening the scene from the root directory of this repo local clone should display a stage with two root prims:

```sh
usdcat --flatten src/arHttp/testenv/stage.usda
```

```
#usda 1.0

def "Prim2" (
    assetInfo = {
        string identifier = "id0x02"
    }
)
{
}

def "Prim1" (
    assetInfo = {
        string identifier = "id0x01"
    }
)
{
}
```

## Acknowledgments

* This projects uses [`httplib`](https://github.com/yhirose/cpp-httplib) from [yhirose](https://github.com/yhirose)
* The idea of a resolver like `arHttp` was suggested by Guillaume Laforge [@frenchdog](https://github.com/frenchdog/) during a Montréal USD Community meetup
* [LucasScheller](https://github.com/LucaScheller) open sourced its [Python resolver](https://github.com/LucaScheller/VFX-UsdAssetResolver), where Python code is directly ran in-process. This other approach allows resolving asset path more directly, without the need of an HTTP server.
