import AbstractView from "./AbstractView.js";

export default class extends AbstractView {
    constructor(params, functions, payload) {
        super(params);
        this.payload = payload;
        this.setTitle("Success");
    }

    async getHtml() {
        return `
            <div class="chip-creation-success container">
                <svg class="checkmark" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 52 52">
                    <circle class="checkmark__circle" cx="26" cy="26" r="25" fill="none"/>
                    <path class="checkmark__check" fill="none" d="M14.1 27.2l7.1 7.2 16.7-16.8"/>
                </svg>


                <h2>
                    ${this.payload?.message ?? "NFC-Device created!"}
                </h2>

                <div class="buttons">
                    <button id="add-card">Add another card</button>
                    <button id="to-home">Return to homepage</button>
                </div>
            </div>
        `;
    }

    onCreated(params, functions, payload) {
        const addCard = document.querySelector("#add-card");
        const toHome = document.querySelector("#to-home");

        addCard.addEventListener("click", () => {
            functions.navigateToPath("/new");
        });
        toHome.addEventListener("click", () => {
            functions.navigateToPath("/");
        });
    }
}