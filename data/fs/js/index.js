import Homepage from "./views/Homepage.js";
import NewChip from "./views/ChipCreation.js";
import ChipSearch from "./views/ChipSearch.js";
import ChipOverview from "./views/ChipOverview.js";
import CreationOk from "./views/CreationOk.js";
import CreationFail from "./views/CreationFail.js";

const pathToRegex = path => new RegExp("^" + path.replace(/\//g, "\\/").replace(/:\w+/g, "(.+)") + "$");

const routes = [
    { path: "/", view: Homepage },
    { path: "/new", view: NewChip },
    { path: "/new/search", view: ChipSearch },
    { path: "/new/success", view: CreationOk },
    { path: "/new/failure", view: CreationFail },
    { path: "/overview", view: ChipOverview },
];

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

const navigateToPath = (path, payload) => {
    navigateTo(window.location.protocol + "//" + window.location.host + path, payload)
};

const functions = {
    navigateTo,
    navigateToPath,
    refresh
}

const router = async (payload) => {
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
