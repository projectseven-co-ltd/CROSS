# Setting Up n8n API Endpoints with NocoDB Backend

This guide walks you through creating custom API endpoints in n8n that interact with your NocoDB database.

---

## Prerequisites
- n8n instance running (cloud or self-hosted)
- NocoDB instance running and accessible
- NocoDB API URL and API token

---

## Step 1: Create a New Workflow in n8n
1. Log in to your n8n dashboard.
2. Click **New Workflow**.

## Step 2: Add a Webhook Trigger
1. Add a **Webhook** node.
2. Set the HTTP method (GET, POST, etc.) and the path (e.g., `/crosses/:cross_id/heartbeat`).
3. Save and copy the webhook URL.

## Step 3: Add HTTP Request Node(s) for NocoDB
1. Add an **HTTP Request** node after the Webhook.
2. Configure it to call your NocoDB API endpoint (e.g., `GET https://your-nocodb/api/v1/db/data/v1/{table}`).
3. Set authentication headers (API token).
4. Use expressions to pass parameters from the webhook (e.g., cross_id).

## Step 4: Add Logic/Processing Nodes (Optional)
- Use **Function** or **Set** nodes to process, filter, or transform data as needed.
- Add additional HTTP Request nodes for multi-step workflows (e.g., update, create, or aggregate data).

## Step 5: Return a Custom Response
1. Add a **Respond to Webhook** node at the end.
2. Map the output data to the response body.

## Step 6: Activate and Test
1. Activate the workflow.
2. Test the endpoint using curl, Postman, or your device/app.

---

## Example: Cross Heartbeat Endpoint
- Webhook: `POST /crosses/:cross_id/heartbeat`
- HTTP Request: PATCH to NocoDB crosses table to update last_heartbeat and status
- Respond to Webhook: Return success or error message

---

Repeat for each custom endpoint. Use n8n’s built-in logic to handle business rules, aggregation, or notifications as needed.
