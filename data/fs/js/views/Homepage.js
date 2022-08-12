import AbstractView from "./AbstractView.js";

export default class extends AbstractView {
    constructor(params) {
        super(params);
        this.setTitle("Homepage");
    }

    async getHtml() {
        return `
            <div class="container dashboard">
                <div class="split left">
                    <h1>Register New Device</h1>
                    <a href="/new" class="button" data-link>To NFC-Wizard</a>
                </div>
                <div class="split right">
                    <h1>Existing Devices</h1>
                    <a href="/overview" class="button">To Device-Manager</a>
                </div>
            </div>
        `;
    }

    onCreated() {
        const left = document.querySelector(".dashboard .left");
        const right = document.querySelector(".dashboard .right");
        const container = document.querySelector(".dashboard.container");

        left.addEventListener("mouseenter", () => {
            container.classList.add("hover-left");
        });

        left.addEventListener("mouseleave", () => {
            container.classList.remove("hover-left");
        });

        right.addEventListener("mouseenter", () => {
            container.classList.add("hover-right");
        });

        right.addEventListener("mouseleave", () => {
            container.classList.remove("hover-right");
        });
    }
}