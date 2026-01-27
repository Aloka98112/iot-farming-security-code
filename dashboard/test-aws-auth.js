
// test-aws-auth.js
require('dotenv').config();
const AWS = require('aws-sdk');

// FIX: Add the region configuration from the main application
const AWS_REGION = 'ap-southeast-2';
AWS.config.update({ region: AWS_REGION });

// We will test the Cognito Identity service directly.
const cognitoIdentity = new AWS.CognitoIdentity();

console.log("Attempting to verify AWS Cognito permissions from .env file...");

// This is a simple read-only action. If the user has basic Cognito permissions, this will succeed.
// If not, it will fail with an AccessDeniedException.
cognitoIdentity.listIdentityPools({ MaxResults: 10 }, (err, data) => {
    if (err) {
        console.error("================================================================");
        console.error("         >>> AWS PERMISSION TEST FAILED <<<");
        console.error("================================================================");
        console.error("The IAM user 'cli-admin' does not have permission for Cognito.");
        console.error("Error Code:   ", err.code);
        console.error("Error Message:", err.message);
        console.error("\nThis is the root cause. The server can authenticate, but it is not");
        console.error("authorized to get credentials from the Cognito Identity service.");
        console.error("\nPlease attach the 'AmazonCognitoPowerUser' or a similar policy to the");
        console.error("'cli-admin' IAM User in the AWS console.");
        console.error("================================================================");
    } else {
        console.log("================================================================");
        console.log("        >>> AWS PERMISSION TEST SUCCESSFUL <<<");
        console.log("================================================================");
        console.log("The IAM user 'cli-admin' has the necessary permissions for Cognito.");
        if (data.IdentityPools && data.IdentityPools.length > 0) {
            console.log("Found Cognito Identity Pools:", data.IdentityPools.map(p => p.PoolName));
        } else {
            console.log("No Cognito Identity Pools found, but the permission check was successful.");
        }
        console.log("================================================================");
    }
});
