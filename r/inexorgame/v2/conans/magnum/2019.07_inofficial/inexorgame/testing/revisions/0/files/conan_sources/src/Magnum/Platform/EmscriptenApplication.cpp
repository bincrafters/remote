/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2018, 2019 Jonathan Hale <squareys@googlemail.com>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "EmscriptenApplication.h"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Debug.h>
#include <Corrade/Utility/String.h>

#include "Magnum/Math/ConfigurationValue.h"

#ifdef MAGNUM_TARGET_GL
#include "Magnum/GL/Version.h"
#include "Magnum/Platform/GLContext.h"
#endif

#include "Magnum/Platform/Implementation/DpiScaling.h"

namespace Magnum { namespace Platform {

namespace {
    typedef EmscriptenApplication::KeyEvent::Key Key;

    /* Entry for key name to `Key` enum mapping */
    struct Entry {
        const char* key;
        Key value;
    };

    /* Key name to `Key` enum mapping. These need to be sorted for use with
       std::lower_bound in KeyEvent::toKey */
    constexpr Entry KeyMapping[]{
        {"AltLeft", Key::LeftAlt},
        {"AltRight", Key::RightAlt},
        {"ArrowDown", Key::Down},
        {"ArrowLeft", Key::Left},
        {"ArrowRight", Key::Right},
        {"ArrowUp", Key::Up},
        {"Backslash", Key::Backslash},
        {"Backspace", Key::Backspace},
        {"CapsLock", Key::CapsLock},
        {"Comma", Key::Comma},
        {"ContextMenu", Key::Menu},
        {"ControlLeft", Key::LeftCtrl},
        {"ControlRight", Key::RightCtrl},
        {"Delete", Key::Delete},
        {"End", Key::End},
        {"Enter", Key::Enter},
        {"Equal", Key::Equal},
        {"Escape", Key::Esc},
        {"Home", Key::Home},
        {"Insert", Key::Insert},
        {"MetaLeft", Key::LeftSuper},
        {"MetaRight", Key::RightSuper},
        {"Minus", Key::Minus},
        {"NumLock", Key::NumLock},
        {"PageDown", Key::PageDown},
        {"PageUp", Key::PageUp},
        {"Pause", Key::Pause},
        {"Period", Key::Period},
        {"Plus", Key::Plus},
        {"PrintScreen", Key::PrintScreen},
        {"Quote", Key::Quote},
        {"ScrollLock", Key::ScrollLock},
        {"ShiftLeft", Key::LeftShift},
        {"ShiftRight", Key::RightShift},
        {"Slash", Key::Slash},
        {"Space", Key::Space},
        {"Tab", Key::Tab},
    };

    /* Predicate for Entry "less than" to use with std::lower_bound */
    struct EntryCompare {
        bool operator()(const Entry& a, const char* const b) {
            return std::strcmp(a.key, b) < 0;
        }

        bool operator()(const char* const a, const Entry& b) {
            return std::strcmp(a, b.key) < 0;
        }
    };

    /* Translate emscripten key code (as defined by
       https://www.w3.org/TR/uievents-code/#key-code-attribute-value)
       to Key enum. `key` is a keyboard layout dependent key string, e.g. 'a',
       or '-'; `code` is a keyboard layout independent key string, e.g. 'KeyA'
       or 'Minus'. Note that the Y key on some layouts may result in 'KeyZ'. */
    Key toKey(const EM_UTF8* const key, const EM_UTF8* const code) {
        const std::size_t keyLength = std::strlen(key);
        if(keyLength == 0) return Key::Unknown;

        /* We use key for a-z as it gives us a keyboard layout respecting
           representation of the key, i.e. we get `z` for z depending on layout
           where code may give us `y` independent of the layout. */
        if(keyLength == 1) {
            if(key[0] >= 'a' && key[0] <= 'z') return Key(key[0]);
            else if(key[0] >= 'A' && key[0] <= 'Z') return Key(key[0] - 'A' + 'a');
        }

        /* We use code for 0-9 as it allows us to differentiate towards Numpad
           digits. For digits independent of numpad or not, key is e.g. '0' for
           Zero */
        const std::size_t codeLength = std::strlen(code);
        if(Utility::String::viewBeginsWith({code, codeLength}, "Digit")) {
            return Key(code[5]);

        /* Numpad keys */
        } else if(Utility::String::viewBeginsWith({code, codeLength}, "Numpad")) {
            std::string numKey(code + 6);
            if(numKey == "Add") return Key::NumAdd;
            if(numKey == "Decimal") return Key::NumDecimal;
            if(numKey == "Divide") return Key::NumDivide;
            if(numKey == "Enter") return Key::NumEnter;
            if(numKey == "Equal") return Key::NumEqual;
            if(numKey == "Multiply") return Key::NumMultiply;
            if(numKey == "Subtract") return Key::NumSubtract;

            /* Numpad0 - Numpad9 */
            const Int num = numKey[0] - '0';
            if(num >= 0 && num <= 9) return Key(num + Int(Key::NumZero));

            return Key::Unknown;
        }

        const Containers::ArrayView<const Entry> mapping = KeyMapping;
        const Entry* found = std::lower_bound(mapping.begin(), mapping.end(), code, EntryCompare{});
        if(found != mapping.end() && std::strcmp(found->key, code) == 0)
            return found->value;

        /* F1 - F12 */
        if(code[0] == 'F') {
            /* F1-F9 */
            if(code[2] != '\0') {
                const Int num = code[2] - '0';
                return Key(Int(Key::F10) + num);
            }
            /* F10-F12 */
            const Int num = code[1] - '1';
            return Key(Int(Key::F1) + num);
        }

        return Key::Unknown;
    }
}

EmscriptenApplication::EmscriptenApplication(const Arguments& arguments): EmscriptenApplication{arguments, Configuration{}} {}

EmscriptenApplication::EmscriptenApplication(const Arguments& arguments, const Configuration& configuration): EmscriptenApplication{arguments, NoCreate} {
    create(configuration);
}

#ifdef MAGNUM_TARGET_GL
EmscriptenApplication::EmscriptenApplication(const Arguments& arguments, const Configuration& configuration, const GLConfiguration& glConfiguration): EmscriptenApplication{arguments, NoCreate} {
    create(configuration, glConfiguration);
}
#endif

EmscriptenApplication::EmscriptenApplication(const Arguments& arguments, NoCreateT):
    _flags{Flag::Redraw}
{
    Utility::Arguments args{Implementation::windowScalingArguments()};
    #ifdef MAGNUM_TARGET_GL
    _context.reset(new GLContext{NoCreate, args, arguments.argc, arguments.argv});
    #else
    args.parse(arguments.argc, arguments.argv);
    #endif

    /* Save command-line arguments */
    if(args.value("log") == "verbose") _verboseLog = true;
    const std::string dpiScaling = args.value("dpi-scaling");

    /* Use physical DPI scaling */
    if(dpiScaling == "default" || dpiScaling == "physical") {

    /* Use explicit dpi scaling vector */
    } else if(dpiScaling.find_first_of(" \t\n") != std::string::npos)
        _commandLineDpiScaling = args.value<Vector2>("dpi-scaling");

    /* Use explicit dpi scaling scalar */
    else
        _commandLineDpiScaling = Vector2{args.value<Float>("dpi-scaling")};
}

EmscriptenApplication::~EmscriptenApplication() {
    #ifdef MAGNUM_TARGET_GL
    _context.reset();

    emscripten_webgl_destroy_context(_glContext);
    #endif
}

void EmscriptenApplication::create() {
    create(Configuration{});
}

void EmscriptenApplication::create(const Configuration& configuration) {
    if(!tryCreate(configuration)) exit(1);
}

#ifdef MAGNUM_TARGET_GL
void EmscriptenApplication::create(const Configuration& configuration, const GLConfiguration& glConfiguration) {
    if(!tryCreate(configuration, glConfiguration)) exit(32);
}
#endif

Vector2 EmscriptenApplication::dpiScaling(const Configuration& configuration) const {
    std::ostream* verbose = _verboseLog ? Debug::output() : nullptr;

    /* Use values from the configuration only if not overriden on command line.
       In any case explicit scaling has a precedence before the policy. */
    if(!_commandLineDpiScaling.isZero()) {
        Debug{verbose} << "Platform::EmscriptenApplication: user-defined DPI scaling" << _commandLineDpiScaling.x();
        return _commandLineDpiScaling;
    } else if(!configuration.dpiScaling().isZero()) {
        Debug{verbose} << "Platform::EmscriptenApplication: app-defined DPI scaling" << _commandLineDpiScaling.x();
        return configuration.dpiScaling();
    }

    /* Unlike Sdl2Application, not taking device pixel ratio into account
       because here we have window size different from framebuffer size.
       However, in order to actually calculate the framebuffer size we need to
       query the device pixel ratio. That's done in tryCreate() below, here it
       is returning 1.0 to be consistent with behavior on other platforms where
       it's either windowSize == 1 */
    return Vector2{1.0f};
}

bool EmscriptenApplication::tryCreate(const Configuration& configuration) {
    #ifdef MAGNUM_TARGET_GL
    if(!(configuration.windowFlags() & Configuration::WindowFlag::Contextless)) {
        return tryCreate(configuration, GLConfiguration{});
    }
    #endif

    _dpiScaling = dpiScaling(configuration);
    if(!configuration.size().isZero()) {
        const Vector2i scaledCanvasSize = configuration.size()*_dpiScaling;
        emscripten_set_canvas_element_size("#canvas", scaledCanvasSize.x(), scaledCanvasSize.y());
    }

    setupCallbacks(!!(configuration.windowFlags() & Configuration::WindowFlag::Resizable));
    setupAnimationFrame(!!(configuration.windowFlags() & Configuration::WindowFlag::AlwaysRequestAnimationFrame));

    return true;
}

#ifdef MAGNUM_TARGET_GL
bool EmscriptenApplication::tryCreate(const Configuration& configuration, const GLConfiguration& glConfiguration) {
    CORRADE_ASSERT(_context->version() == GL::Version::None, "Platform::EmscriptenApplication::tryCreate(): window with OpenGL context already created", false);

    /* Create emscripten WebGL context */
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = glConfiguration.colorBufferSize().a() > 0;
    attrs.depth = glConfiguration.depthBufferSize() > 0;
    attrs.stencil = glConfiguration.stencilBufferSize() > 0;
    attrs.antialias = glConfiguration.sampleCount() > 0;

    attrs.premultipliedAlpha =
        !!(glConfiguration.flags() & GLConfiguration::Flag::PremultipliedAlpha);
    attrs.preserveDrawingBuffer =
        !!(glConfiguration.flags() & GLConfiguration::Flag::PreserveDrawingBuffer);
    /* powerPreference replaced preferLowPowerToHighPerformance in emscripten
       version 1.38.26 */
    #ifdef EM_WEBGL_POWERPREFERENCE_LOW_POWER
    attrs.powerPreference =
        !!(glConfiguration.flags() & GLConfiguration::Flag::PreferLowPowerToHighPerformance)
        ? EM_WEBGL_POWERPREFERENCE_LOW_POWER : EM_WEBGL_POWERPREFERENCE_HIGH_PERFORMANCE;
    #else
    attrs.preferLowPowerToHighPerformance =
        !!(glConfiguration.flags() & GLConfiguration::Flag::PreferLowPowerToHighPerformance);
    #endif
    attrs.explicitSwapControl =
        !!(glConfiguration.flags() & GLConfiguration::Flag::ExplicitSwapControl);
    attrs.failIfMajorPerformanceCaveat =
        !!(glConfiguration.flags() & GLConfiguration::Flag::FailIfMajorPerformanceCaveat);
    attrs.enableExtensionsByDefault =
        !!(glConfiguration.flags() & GLConfiguration::Flag::EnableExtensionsByDefault);

    #ifdef MAGNUM_TARGET_GLES3 /* WebGL 2 */
    attrs.majorVersion = 2;
    #elif defined(MAGNUM_TARGET_GLES2) /* WebGL 1 */
    attrs.minorVersion = 1;
    #else
    #error unsupported OpenGL ES version
    #endif

    std::ostream* verbose = _verboseLog ? Debug::output() : nullptr;

    /* Fetch device pixel ratio. Together with DPI scaling (which is 1.0 by
       default) this will define framebuffer size. See class docs for why is it
       done like that. */
    _devicePixelRatio = Vector2{Float(emscripten_get_device_pixel_ratio())};
    Debug{verbose} << "Platform::EmscriptenApplication: device pixel ratio" << _devicePixelRatio.x();

    /* Get CSS canvas size and cache it. This is used later to detect canvas
       resizes in emscripten_set_resize_callback() and fire viewport events,
       because browsers are only required to fire resize events on the window
       and not on particular DOM elements. */
    _lastKnownCanvasSize = windowSize();

    /* By default Emscripten creates a 300x150 canvas. That's so freaking
       random I'm getting mad. Use the real (CSS pixels) canvas size instead,
       if the size is not hardcoded from the configuration. This is then
       multiplied by the DPI scaling. */
    Vector2i canvasSize;
    if(!configuration.size().isZero()) {
        canvasSize = configuration.size();
    } else {
        canvasSize = _lastKnownCanvasSize;
        Debug{verbose} << "Platform::EmscriptenApplication::tryCreate(): autodetected canvas size" << canvasSize;
    }
    _dpiScaling = dpiScaling(configuration);
    const Vector2i scaledCanvasSize = canvasSize*_dpiScaling*_devicePixelRatio;
    emscripten_set_canvas_element_size("#canvas", scaledCanvasSize.x(), scaledCanvasSize.y());

    /* Create WebGL context */
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context("#canvas", &attrs);
    if(!context) {
        /* When context creation fails, `context` is a negative integer
           matching EMSCRIPTEN_RESULT_* defines */
        Error{} << "Platform::EmscriptenApplication::tryCreate(): cannot create WebGL context (EMSCRIPTEN_RESULT"
                << context << Debug::nospace << ")";
        return false;
    }

    /* Make the context current */
    CORRADE_INTERNAL_ASSERT_OUTPUT(emscripten_webgl_make_context_current(_glContext = context) == EMSCRIPTEN_RESULT_SUCCESS);

    setupCallbacks(!!(configuration.windowFlags() & Configuration::WindowFlag::Resizable));
    setupAnimationFrame(!!(configuration.windowFlags() & Configuration::WindowFlag::AlwaysRequestAnimationFrame));

    /* Return true if the initialization succeeds */
    return _context->tryCreate();
}
#endif

Vector2i EmscriptenApplication::windowSize() const {
    Vector2d size;
    /* Emscripten 1.38.27 changed to generic CSS selectors from element IDs
       depending on -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1 being
       set (which we can't detect at compile time). Fortunately, using #canvas
       works the same way both in the previous versions and the current one.
       Unfortunately, this is also the only value that works the same way for
       both. Further details at
       https://github.com/emscripten-core/emscripten/pull/7977 */
    /** @todo don't hardcode "#canvas" everywhere, make it configurable from outside */
    emscripten_get_element_css_size("#canvas", &size.x(), &size.y());
    return Vector2i{Math::round(size)};
}

#ifdef MAGNUM_TARGET_GL
Vector2i EmscriptenApplication::framebufferSize() const {
    Vector2i size;
    /* See above why hardcoded */
    emscripten_get_canvas_element_size("#canvas", &size.x(), &size.y());
    return size;
}
#endif

void EmscriptenApplication::setContainerCssClass(const std::string& cssClass) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
    EM_ASM_({document.getElementById('container').className = AsciiToString($0);}, cssClass.data());
    #pragma GCC diagnostic pop

    /* Trigger a potential viewport event -- we don't poll the canvas size like
       Sdl2Application does, so it needs to be done explicitly */
    handleCanvasResize(nullptr);
}

void EmscriptenApplication::swapBuffers() {
    emscripten_webgl_commit_frame();
}

/* Called from window resize event but also explicitly from
   setContainerCssClass() */
void EmscriptenApplication::handleCanvasResize(const EmscriptenUiEvent* event) {
    /* See windowSize() for why we hardcode "#canvas" here */
    const Vector2i canvasSize{windowSize()};
    if(canvasSize != _lastKnownCanvasSize) {
        _lastKnownCanvasSize = canvasSize;
        const Vector2i size = canvasSize*_dpiScaling*_devicePixelRatio;
        emscripten_set_canvas_element_size("#canvas", size.x(), size.y());
        ViewportEvent e{event, canvasSize,
            #ifdef MAGNUM_TARGET_GL
            framebufferSize(),
            #endif
            _dpiScaling, _devicePixelRatio};
        viewportEvent(e);

        /* Can't say just _flags | Flag::Redraw because in case the
           requestAnimationFrame callback is not set up at the moment it would
           never up this change. */
        redraw();
    }
}

void EmscriptenApplication::setupCallbacks(bool resizable) {
    /* Since 1.38.17 all emscripten_set_*_callback() are macros. Play it safe
       and wrap all lambdas in () to avoid the preprocessor getting upset when
       seeing commas */

    /* Set up the resize callback. Because browsers are only required to fire
       resize events on the window and not on particular DOM elements, we need
       to cache the last known canvas size and fire the event only if that
       changes. Better than polling for this change in every frame like
       Sdl2Application does, but still not ideal. */
    if(resizable) {
        #ifdef EMSCRIPTEN_EVENT_TARGET_WINDOW
        const char* target = EMSCRIPTEN_EVENT_TARGET_WINDOW;
        #else
        const char* target = "#window";
        #endif
        auto cb = [](int, const EmscriptenUiEvent* event, void* userData) -> Int {
            EmscriptenApplication& app = *static_cast<EmscriptenApplication*>(userData);
            app.handleCanvasResize(event);
            return false; /** @todo what does ignoring a resize event mean? */
        };
        emscripten_set_resize_callback(target, this, false, cb);
    }

    /* See windowSize() for why we hardcode "#canvas" here */

    emscripten_set_mousedown_callback("#canvas", this, false,
        ([](int, const EmscriptenMouseEvent* event, void* userData) -> Int {
            MouseEvent e{*event};
            static_cast<EmscriptenApplication*>(userData)->mousePressEvent(e);
            return e.isAccepted();
        }));

    emscripten_set_mouseup_callback("#canvas", this, false,
        ([](int, const EmscriptenMouseEvent* event, void* userData) -> Int {
            MouseEvent e{*event};
            static_cast<EmscriptenApplication*>(userData)->mouseReleaseEvent(e);
            return e.isAccepted();
        }));

    emscripten_set_mousemove_callback("#canvas", this, false,
        ([](int, const EmscriptenMouseEvent* event, void* userData) -> Int {
            MouseMoveEvent e{*event};
            static_cast<EmscriptenApplication*>(userData)->mouseMoveEvent(e);
            return e.isAccepted();
        }));

    emscripten_set_wheel_callback("#canvas", this, false,
        ([](int, const EmscriptenWheelEvent* event, void* userData) -> Int {
            MouseScrollEvent e{*event};
            static_cast<EmscriptenApplication*>(userData)->mouseScrollEvent(e);
            return e.isAccepted();
        }));

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
    /* document and window are 'specialEventTargets' in emscripten, matching
       EMSCRIPTEN_EVENT_TARGET_DOCUMENT and EMSCRIPTEN_EVENT_TARGET_WINDOW.
       As the lookup happens with the passed parameter and arrays support
       element lookup via strings, we can unify the code by returning string of
       1 or 2 if the target is document or window. This changed in Emscripten
       1.38.27 depending on -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1
       but we don't want to force this flag on the users so the behavior
       handles both. */
    #ifdef EMSCRIPTEN_EVENT_TARGET_DOCUMENT
    char* const keyboardListeningElement = reinterpret_cast<char*>(EM_ASM_INT({
        var element = Module['keyboardListeningElement'] || document;

        if(element === document) return 1;
        if(element === window) return 2;
        if('id' in element)
            return allocate(intArrayFromString(element.id), 'i8', ALLOC_NORMAL);

        return 0;
    }));
    #else
    char* const keyboardListeningElement = reinterpret_cast<char*>(EM_ASM_INT({
        var element = Module['keyboardListeningElement'] || document;

        if(element === document) element = {id: '#document'};
        if(element === window) element = {id: '#window'};
        if('id' in element)
            return allocate(intArrayFromString(element.id), 'i8', ALLOC_NORMAL);

        return 0;
    }));
    #endif
    #pragma GCC diagnostic pop

    /* Happens only if keyboardListeningElement was set, but did not have an
       `id` attribute. Instead it should be either null or undefined, a DOM
       element, `window` or `document`. */
    CORRADE_ASSERT(keyboardListeningElement,
        "EmscriptenApplication::setupCallbacks(): invalid value for Module['keyboardListeningElement']", );

    /* keypress_callback does not fire for most of the keys and the modifiers
       don't seem to work, keydown on the other hand works fine for all */
    emscripten_set_keydown_callback(keyboardListeningElement, this, false,
        ([](int, const EmscriptenKeyboardEvent* event, void* userData) -> Int {
            EmscriptenApplication& app = *static_cast<EmscriptenApplication*>(userData);
            const std::size_t keyLen = std::strlen(event->key);
            /* If the key name is a single letter or a start of an UTF-8
               sequence, pass it to the text input even tas well */
            if(app.isTextInputActive() && (std::strlen(event->key) == 1 || (std::strlen(event->key) >= 1 && UnsignedByte(event->key[0]) > 127))) {
                TextInputEvent e{*event, {event->key, keyLen}};
                app.textInputEvent(e);
                return e.isAccepted();
            }
            KeyEvent e{*event};
            app.keyPressEvent(e);
            return e.isAccepted();
        }));

    emscripten_set_keyup_callback(keyboardListeningElement, this, false,
        ([](int, const EmscriptenKeyboardEvent* event, void* userData) -> Int {
            KeyEvent e{*event};
            static_cast<EmscriptenApplication*>(userData)->keyReleaseEvent(e);
            return e.isAccepted();
        }));

    #ifdef EMSCRIPTEN_EVENT_TARGET_DOCUMENT
    if(keyboardListeningElement != EMSCRIPTEN_EVENT_TARGET_DOCUMENT &&
       keyboardListeningElement != EMSCRIPTEN_EVENT_TARGET_WINDOW)
    #endif
    {
        std::free(keyboardListeningElement);
    }
}


void EmscriptenApplication::setupAnimationFrame(bool forceAnimationFrame) {
    if(forceAnimationFrame) {
        _callback = [](void* userData) -> int {
            auto& app = *static_cast<EmscriptenApplication*>(userData);

            if(app._flags & Flag::ExitRequested) {
                app._flags &= ~Flag::LoopActive;
                return false;
            }

            if(app._flags & Flag::Redraw) {
                app._flags &= ~Flag::Redraw;
                app.drawEvent();
            }

            return true;
        };
    } else {
        _callback = [](void* userData) -> int {
            auto& app = *static_cast<EmscriptenApplication*>(userData);

            if((app._flags & Flag::Redraw) && !(app._flags & Flag::ExitRequested)) {
                app._flags &= ~Flag::Redraw;
                app.drawEvent();
            }

            /* If redraw is requested, we will not cancel the already requested
               animation frame. If ForceAnimationFrame is set, we will request
               an animation frame even if redraw is not requested. */
            if((app._flags & Flag::Redraw) && !(app._flags & Flag::ExitRequested)) {
                return true;
            }

            /* Cancel last requested animation frame and make redraw()
               requestAnimationFrame again next time */
            app._flags &= ~Flag::LoopActive;
            return false;
        };
    }
}

void EmscriptenApplication::startTextInput() {
    _flags |= Flag::TextInputActive;
}

void EmscriptenApplication::stopTextInput() {
    _flags &= ~Flag::TextInputActive;
}

void EmscriptenApplication::setTextInputRect(const Range2Di&) {
    /** @todo place a hidden input field at given rect */
}

void EmscriptenApplication::viewportEvent(ViewportEvent&) {}
void EmscriptenApplication::keyPressEvent(KeyEvent&) {}
void EmscriptenApplication::keyReleaseEvent(KeyEvent&) {}
void EmscriptenApplication::mousePressEvent(MouseEvent&) {}
void EmscriptenApplication::mouseReleaseEvent(MouseEvent&) {}
void EmscriptenApplication::mouseMoveEvent(MouseMoveEvent&) {}
void EmscriptenApplication::mouseScrollEvent(MouseScrollEvent&) {}
void EmscriptenApplication::textInputEvent(TextInputEvent&) {}

#ifdef MAGNUM_TARGET_GL
EmscriptenApplication::GLConfiguration::GLConfiguration():
    _colorBufferSize{8, 8, 8, 0}, _depthBufferSize{24}, _stencilBufferSize{0} {}
#endif

int EmscriptenApplication::exec() {
    redraw();
    return 0;
}

void EmscriptenApplication::redraw() {
    _flags |= Flag::Redraw;

    /* Loop already running, no need to start,
       Note that should javascript runtimes ever be multithreaded, we
       will have a reentrancy issue here. */
    if(_flags & Flag::LoopActive) return;

    /* Start requestAnimationFrame loop */
    _flags |= Flag::LoopActive;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
    EM_ASM({
        /* Animation frame callback */
        var drawEvent = function() {
            var id = window.requestAnimationFrame(drawEvent);

            /* Call our callback via function pointer returning int with two
            int params */
            if(!dynCall('ii', $0, [$1])) {
                window.cancelAnimationFrame(id);
            }
        };

        window.requestAnimationFrame(drawEvent);
    }, _callback, this);
    #pragma GCC diagnostic pop
}

void EmscriptenApplication::exit(int) {
    _flags |= Flag::ExitRequested;
}

EmscriptenApplication::MouseEvent::Button EmscriptenApplication::MouseEvent::button() const {
    return Button(_event.button);
}

Vector2i EmscriptenApplication::MouseEvent::position() const {
    return {Int(_event.canvasX), Int(_event.canvasY)};
}

EmscriptenApplication::MouseEvent::Modifiers EmscriptenApplication::MouseEvent::modifiers() const {
    Modifiers m;
    if(_event.ctrlKey) m |= Modifier::Ctrl;
    if(_event.shiftKey) m |= Modifier::Shift;
    if(_event.altKey) m |= Modifier::Alt;
    if(_event.metaKey) m |= Modifier::Super;
    return m;
}

EmscriptenApplication::MouseMoveEvent::Buttons EmscriptenApplication::MouseMoveEvent::buttons() const {
    return EmscriptenApplication::MouseMoveEvent::Button(_event.buttons);
}

Vector2i EmscriptenApplication::MouseMoveEvent::position() const {
    return {Int(_event.canvasX), Int(_event.canvasY)};
}

EmscriptenApplication::MouseMoveEvent::Modifiers EmscriptenApplication::MouseMoveEvent::modifiers() const {
    Modifiers m;
    if(_event.ctrlKey) m |= Modifier::Ctrl;
    if(_event.shiftKey) m |= Modifier::Shift;
    if(_event.altKey) m |= Modifier::Alt;
    if(_event.metaKey) m |= Modifier::Super;
    return m;
}

Vector2 EmscriptenApplication::MouseScrollEvent::offset() const {
    /* From emscripten's Browser.getMouseWheelDelta() function in
       library_browser.js:

       DOM_DELTA_PIXEL => 100 pixels = 1 step
       DOM_DELTA_LINE => 3 lines = 1 step
       DOM_DELTA_PAGE => 1 page = 80 steps */
    const Float f = (_event.deltaMode == DOM_DELTA_PIXEL) ? -0.01f :
        ((_event.deltaMode == DOM_DELTA_LINE) ? -1.0f/3.0f : -80.0f);

    return {f*Float(_event.deltaX), f*Float(_event.deltaY)};
}

Vector2i EmscriptenApplication::MouseScrollEvent::position() const {
    return {Int(_event.mouse.canvasX), Int(_event.mouse.canvasY)};
}

EmscriptenApplication::InputEvent::Modifiers EmscriptenApplication::MouseScrollEvent::modifiers() const {
    Modifiers m;
    if(_event.mouse.ctrlKey) m |= Modifier::Ctrl;
    if(_event.mouse.shiftKey) m |= Modifier::Shift;
    if(_event.mouse.altKey) m |= Modifier::Alt;
    if(_event.mouse.metaKey) m |= Modifier::Super;
    return m;
}

Key EmscriptenApplication::KeyEvent::key() const {
    return toKey(_event.key, _event.code);
}

std::string EmscriptenApplication::KeyEvent::keyName() const {
    if((_event.key[0] >= 'a' && _event.key[0] <= 'z') ||
       (_event.key[0] >= 'A' && _event.key[0] <= 'Z')) return _event.key;

    return _event.code;
}

EmscriptenApplication::InputEvent::Modifiers EmscriptenApplication::KeyEvent::modifiers() const {
    Modifiers m;
    if(_event.ctrlKey) m |= Modifier::Ctrl;
    if(_event.shiftKey) m |= Modifier::Shift;
    if(_event.altKey) m |= Modifier::Alt;
    if(_event.metaKey) m |= Modifier::Super;
    return m;
}

}}
