export default class {
    constructor(params, refresh, payload) {
        this.params = params;
    }

    setTitle(title) {
        document.title = title;
    }

    async getHtml() {
        return "";
    }

    onCreated() {
    }
}