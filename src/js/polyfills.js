// Polyfills for QtWebKit 5.212 (Safari 10 / JSC ~ES2016)
// Injected before page scripts via javaScriptWindowObjectCleared

// NodeList.forEach
if (window.NodeList && !NodeList.prototype.forEach) {
    NodeList.prototype.forEach = Array.prototype.forEach;
}

// Object.values
if (!Object.values) {
    Object.values = function(obj) {
        return Object.keys(obj).map(function(key) { return obj[key]; });
    };
}

// Object.entries
if (!Object.entries) {
    Object.entries = function(obj) {
        return Object.keys(obj).map(function(key) { return [key, obj[key]]; });
    };
}

// URLSearchParams (minimal, covers htmx usage)
if (typeof URLSearchParams === 'undefined') {
    window.URLSearchParams = function(init) {
        this._params = {};
        if (typeof init === 'string') {
            init = init.replace(/^\?/, '');
            var pairs = init.split('&');
            for (var i = 0; i < pairs.length; i++) {
                var pair = pairs[i].split('=');
                if (pair[0]) {
                    this._params[decodeURIComponent(pair[0])] = decodeURIComponent(pair[1] || '');
                }
            }
        } else if (init && typeof init === 'object') {
            var keys = Object.keys(init);
            for (var j = 0; j < keys.length; j++) {
                this._params[keys[j]] = String(init[keys[j]]);
            }
        }
    };
    URLSearchParams.prototype.get = function(name) {
        return this._params.hasOwnProperty(name) ? this._params[name] : null;
    };
    URLSearchParams.prototype.set = function(name, value) {
        this._params[name] = String(value);
    };
    URLSearchParams.prototype.has = function(name) {
        return this._params.hasOwnProperty(name);
    };
    URLSearchParams.prototype.delete = function(name) {
        delete this._params[name];
    };
    URLSearchParams.prototype.append = function(name, value) {
        this._params[name] = String(value);
    };
    URLSearchParams.prototype.toString = function() {
        var parts = [];
        var keys = Object.keys(this._params);
        for (var i = 0; i < keys.length; i++) {
            parts.push(encodeURIComponent(keys[i]) + '=' + encodeURIComponent(this._params[keys[i]]));
        }
        return parts.join('&');
    };
    URLSearchParams.prototype.forEach = function(callback, thisArg) {
        var keys = Object.keys(this._params);
        for (var i = 0; i < keys.length; i++) {
            callback.call(thisArg, this._params[keys[i]], keys[i], this);
        }
    };
}

// Array.prototype.flat (used by some htmx extensions)
if (!Array.prototype.flat) {
    Array.prototype.flat = function(depth) {
        depth = depth === undefined ? 1 : Math.floor(depth);
        if (depth < 1) return Array.prototype.slice.call(this);
        return Array.prototype.reduce.call(this, function(acc, val) {
            if (Array.isArray(val) && depth > 0) {
                return acc.concat(Array.prototype.flat.call(val, depth - 1));
            }
            return acc.concat(val);
        }, []);
    };
}

// String.prototype.trimStart / trimEnd
if (!String.prototype.trimStart) {
    String.prototype.trimStart = function() { return this.replace(/^\s+/, ''); };
}
if (!String.prototype.trimEnd) {
    String.prototype.trimEnd = function() { return this.replace(/\s+$/, ''); };
}
