import AbstractView from "./AbstractView.js";

export default class extends AbstractView {
    constructor(params, functions, payload) {
        super(params);
        this.payload = payload;
        this.setTitle("New Chip");
    }

    async getHtml() {
        return `
            <div class="new-chip container">
                <h2>
                    Give your new NFC-device a name:
                </h2>

                <form class="submit">
                    <input id="name" placeholder="Name" type="text" required="">
                    <button id="submit">Submit</button>
                </form>
            </div>
        `;
    }

    onCreated(params, functions, payload) {
        const form = document.querySelector("form.submit");
        const name = document.querySelector("form.submit input#name");

        form.addEventListener("submit", (e) => {
            e.preventDefault();
            functions.navigateToPath("/new/search", {name: name.value});
        });
    }
}