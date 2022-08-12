import Homepage from "./views/Homepage.js";
import NewChip from "./views/NewChip.js";
import ChipSearch from "./views/ChipSearch.js";
import ChipFound from "./views/ChipCreated.js";
import ChipOverview from "./views/ChipOverview.js";

const pathToRegex = path => new RegExp("^" + path.replace(/\//g, "\\/").replace(/:\w+/g, "(.+)") + "$");

const getParams = match => {
    const values = match.result.slice(1);
    const keys = Array.from(match.route.path.matchAll(/:(\w+)/g)).map(result => result[1]);

    return Object.fromEntries(keys.map((key, i) => {
        return [key, values[i]];
    }));
};

const refresh = (payload) => {
    router(payload);
}

const navigateTo = (url, payload) => {
    history.pushState(null, null, url);
    router(payload);
};

const functions = {
    navigateTo,
    refresh
}

const router = async (payload) => {
    const routes = [
        { path: "/", view: Homepage },
        { path: "/new", view: NewChip },
        { path: "/chipSearch", view: ChipSearch },
        { path: "/chipFound", view: ChipFound },
        { path: "/overview", view: ChipOverview },
    ];

    // Test each route for potential match
    const potentialMatches = routes.map(route => {
        return {
            route: route,
            result: location.pathname.match(pathToRegex(route.path))
        };
    });

    let match = potentialMatches.find(potentialMatch => potentialMatch.result !== null);

    if (!match) {
        match = {
            route: routes[0],
            result: [location.pathname]
        };
    }

    const view = new match.route.view(getParams(match), functions, payload);

    document.querySelector("#app").innerHTML = await view.getHtml();

    view.onCreated(getParams(match), functions, payload);
};

window.addEventListener("popstate", router);

document.addEventListener("DOMContentLoaded", () => {
    document.body.addEventListener("click", e => {
        if (e.target.matches("[data-link]")) {
            e.preventDefault();
            navigateTo(e.target.href);
        }
    });

    router();
});
